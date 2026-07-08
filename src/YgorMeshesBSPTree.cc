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
#include <optional>
#include <set>
#include <stdexcept>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorMeshesAdaptivePredicates.h"
#include "YgorMeshesBSPTree.h"
#include "YgorMeshesOrient.h"
#include "YgorMeshesVerification.h"


namespace {

using ExactInt = boost::multiprecision::cpp_int;

ExactInt abs_exact_int(ExactInt v) {
    return (v < 0) ? -v : v;
}

ExactInt gcd_exact_int(ExactInt a, ExactInt b) {
    a = abs_exact_int(a);
    b = abs_exact_int(b);
    while(b != 0) {
        const ExactInt r = a % b;
        a = b;
        b = r;
    }
    return (a == 0) ? ExactInt(1) : a;
}

struct ExactScalar {
    ExactInt n = 0;
    ExactInt d = 1;

    ExactScalar() = default;
    ExactScalar(int64_t v) : n(v), d(1) {}
    ExactScalar(ExactInt num, ExactInt den) : n(std::move(num)), d(std::move(den)) {
        normalize();
    }

    template <class T>
    static ExactScalar from_binary(T value) {
        static_assert(std::numeric_limits<T>::is_iec559, "ExactScalar requires IEC 559 floating point input.");
        if(!std::isfinite(value)) {
            throw std::invalid_argument("ExactScalar::from_binary: non-finite input.");
        }
        if(value == static_cast<T>(0)) return ExactScalar();

        int exp = 0;
        const T frac = std::frexp(value, &exp);
        const int digits = std::numeric_limits<T>::digits;
        const T scaled = std::ldexp(frac, digits);
        const auto mantissa = static_cast<int64_t>(scaled);

        ExactInt num = mantissa;
        ExactInt den = 1;
        const int pow2 = exp - digits;
        if(pow2 >= 0) {
            num <<= pow2;
        } else {
            den <<= -pow2;
        }
        return ExactScalar(std::move(num), std::move(den));
    }

    void normalize() {
        if(d == 0) throw std::invalid_argument("ExactScalar: zero denominator.");
        if(d < 0) {
            n = -n;
            d = -d;
        }
        const ExactInt g = gcd_exact_int(n, d);
        n /= g;
        d /= g;
    }

    int sign() const {
        return (n > 0) ? +1 : ((n < 0) ? -1 : 0);
    }

    bool is_zero() const {
        return n == 0;
    }
};

ExactScalar operator-(const ExactScalar &a) {
    return ExactScalar(-a.n, a.d);
}

ExactScalar operator+(const ExactScalar &a, const ExactScalar &b) {
    return ExactScalar(a.n * b.d + b.n * a.d, a.d * b.d);
}

ExactScalar operator-(const ExactScalar &a, const ExactScalar &b) {
    return ExactScalar(a.n * b.d - b.n * a.d, a.d * b.d);
}

ExactScalar operator*(const ExactScalar &a, const ExactScalar &b) {
    return ExactScalar(a.n * b.n, a.d * b.d);
}

ExactScalar operator/(const ExactScalar &a, const ExactScalar &b) {
    if(b.n == 0) throw std::invalid_argument("ExactScalar division by zero.");
    return ExactScalar(a.n * b.d, a.d * b.n);
}

bool operator==(const ExactScalar &a, const ExactScalar &b) {
    return a.n == b.n && a.d == b.d;
}

bool operator!=(const ExactScalar &a, const ExactScalar &b) {
    return !(a == b);
}

bool operator<(const ExactScalar &a, const ExactScalar &b) {
    return a.n * b.d < b.n * a.d;
}

bool operator<=(const ExactScalar &a, const ExactScalar &b) {
    return !(b < a);
}

struct ExactPoint2 {
    ExactScalar x;
    ExactScalar y;
};

ExactPoint2 operator+(const ExactPoint2 &a, const ExactPoint2 &b) {
    return { a.x + b.x, a.y + b.y };
}

ExactPoint2 operator-(const ExactPoint2 &a, const ExactPoint2 &b) {
    return { a.x - b.x, a.y - b.y };
}

ExactPoint2 operator*(const ExactPoint2 &p, const ExactScalar &s) {
    return { p.x * s, p.y * s };
}

bool operator==(const ExactPoint2 &a, const ExactPoint2 &b) {
    return a.x == b.x && a.y == b.y;
}

bool exact_point2_less(const ExactPoint2 &a, const ExactPoint2 &b) {
    return std::tie(a.x, a.y) < std::tie(b.x, b.y);
}

ExactScalar cross_exact_2d(const ExactPoint2 &a, const ExactPoint2 &b) {
    return a.x * b.y - a.y * b.x;
}

ExactScalar orient_exact_2d(const ExactPoint2 &a,
                            const ExactPoint2 &b,
                            const ExactPoint2 &c) {
    return cross_exact_2d(b - a, c - a);
}

struct ExactPoint3 {
    ExactScalar x;
    ExactScalar y;
    ExactScalar z;

    template <class T>
    static ExactPoint3 from_vec3(const vec3<T> &v) {
        return { ExactScalar::from_binary(v.x),
                 ExactScalar::from_binary(v.y),
                 ExactScalar::from_binary(v.z) };
    }
};

ExactPoint3 operator+(const ExactPoint3 &a, const ExactPoint3 &b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

ExactPoint3 operator-(const ExactPoint3 &a, const ExactPoint3 &b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

ExactPoint3 operator*(const ExactPoint3 &p, const ExactScalar &s) {
    return { p.x * s, p.y * s, p.z * s };
}

ExactScalar dot_exact(const ExactPoint3 &a, const ExactPoint3 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

ExactPoint3 cross_exact(const ExactPoint3 &a, const ExactPoint3 &b) {
    return { a.y * b.z - a.z * b.y,
             a.z * b.x - a.x * b.z,
             a.x * b.y - a.y * b.x };
}

struct ExactPlane3 {
    ExactScalar a;
    ExactScalar b;
    ExactScalar c;
    ExactScalar d;

    static ExactPlane3 from_points(const ExactPoint3 &p0,
                                   const ExactPoint3 &p1,
                                   const ExactPoint3 &p2) {
        const ExactPoint3 n = cross_exact(p1 - p0, p2 - p0);
        if(n.x.is_zero() && n.y.is_zero() && n.z.is_zero()) {
            throw std::invalid_argument("ExactPlane3::from_points: degenerate plane.");
        }
        return { n.x, n.y, n.z, -dot_exact(n, p0) };
    }

    ExactScalar eval(const ExactPoint3 &p) const {
        return a * p.x + b * p.y + c * p.z + d;
    }
};

struct ExactLine3 {
    ExactPoint3 p;
    ExactPoint3 dir;
};

struct CanonicalPlaneKey {
    std::array<ExactInt, 4> coeffs = {{0, 0, 0, 0}};
};

CanonicalPlaneKey canonical_plane_key(const ExactPlane3 &plane);

template <class T>
CanonicalPlaneKey canonical_plane_key(const bsp_plane<T> &plane);

enum class ExactVertexKind : uint8_t {
    OriginalVertex = 0,
    OriginalEdgeSplit = 1,
    PlaneTriple = 2,
    Coordinate = 3
};

struct ExactVertexSymbol {
    ExactVertexKind kind = ExactVertexKind::Coordinate;
    std::array<uint64_t, 3> ids = {{0, 0, 0}};
};

struct ExactVertex3 {
    ExactPoint3 p;
    ExactVertexSymbol symbol;
};

bool operator<(const ExactVertexSymbol &a, const ExactVertexSymbol &b) {
    return std::tie(a.kind, a.ids) < std::tie(b.kind, b.ids);
}

bool operator==(const ExactVertexSymbol &a, const ExactVertexSymbol &b) {
    return a.kind == b.kind && a.ids == b.ids;
}

bool operator!=(const ExactVertexSymbol &a, const ExactVertexSymbol &b) {
    return !(a == b);
}

bool same_symbolic_identity(const ExactVertexSymbol &a, const ExactVertexSymbol &b) {
    return a.kind == b.kind && a.kind != ExactVertexKind::Coordinate && a.ids == b.ids;
}

bool exact_point_less(const ExactPoint3 &a, const ExactPoint3 &b) {
    return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
}

bool operator==(const ExactPoint3 &a, const ExactPoint3 &b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool exact_vertex_less(const ExactVertex3 &a, const ExactVertex3 &b) {
    if(same_symbolic_identity(a.symbol, b.symbol)) return false;
    if(a.symbol.kind != ExactVertexKind::Coordinate || b.symbol.kind != ExactVertexKind::Coordinate) {
        if(a.symbol != b.symbol) return a.symbol < b.symbol;
    }
    return exact_point_less(a.p, b.p);
}

ExactPoint3 intersect_line_plane_exact(const ExactLine3 &line, const ExactPlane3 &plane) {
    const ExactScalar denom = plane.a * line.dir.x + plane.b * line.dir.y + plane.c * line.dir.z;
    if(denom.is_zero()) throw std::invalid_argument("intersect_line_plane_exact: parallel line and plane.");
    const ExactScalar t = -plane.eval(line.p) / denom;
    return line.p + line.dir * t;
}

ExactPoint3 intersect_segment_plane_exact(const ExactPoint3 &a,
                                          const ExactPoint3 &b,
                                          const ExactPlane3 &plane) {
    const ExactScalar ea = plane.eval(a);
    const ExactScalar eb = plane.eval(b);
    if(ea.is_zero()) return a;
    if(eb.is_zero()) return b;
    if(ea.sign() == eb.sign()) {
        throw std::invalid_argument("intersect_segment_plane_exact: segment does not cross plane.");
    }
    return intersect_line_plane_exact({a, b - a}, plane);
}

ExactLine3 intersect_plane_plane_exact(const ExactPlane3 &p,
                                       const ExactPlane3 &q) {
    const ExactPoint3 n1{p.a, p.b, p.c};
    const ExactPoint3 n2{q.a, q.b, q.c};
    const ExactPoint3 dir = cross_exact(n1, n2);
    if(dir.x.is_zero() && dir.y.is_zero() && dir.z.is_zero()) {
        throw std::invalid_argument("intersect_plane_plane_exact: parallel planes.");
    }

    const ExactScalar abs_x = dir.x.sign() < 0 ? -dir.x : dir.x;
    const ExactScalar abs_y = dir.y.sign() < 0 ? -dir.y : dir.y;
    const ExactScalar abs_z = dir.z.sign() < 0 ? -dir.z : dir.z;
    if(!(abs_x < abs_y) && !(abs_x < abs_z)) {
        const ExactScalar det = p.b * q.c - q.b * p.c;
        return { { ExactScalar(), (p.c * q.d - q.c * p.d) / det, (q.b * p.d - p.b * q.d) / det }, dir };
    }
    if(!(abs_y < abs_z)) {
        const ExactScalar det = p.a * q.c - q.a * p.c;
        return { { (p.c * q.d - q.c * p.d) / det, ExactScalar(), (q.a * p.d - p.a * q.d) / det }, dir };
    }
    const ExactScalar det = p.a * q.b - q.a * p.b;
    return { { (p.b * q.d - q.b * p.d) / det, (q.a * p.d - p.a * q.d) / det, ExactScalar() }, dir };
}

ExactPoint3 intersect_plane_plane_plane_exact(const ExactPlane3 &p,
                                              const ExactPlane3 &q,
                                              const ExactPlane3 &r) {
    const ExactLine3 line = intersect_plane_plane_exact(p, q);
    return intersect_line_plane_exact(line, r);
}

template <class T>
int filtered_orient3d_sign(const vec3<T> &a,
                           const vec3<T> &b,
                           const vec3<T> &c,
                           const vec3<T> &d) {
    const T pa[3] = { a.x, a.y, a.z };
    const T pb[3] = { b.x, b.y, b.z };
    const T pc[3] = { c.x, c.y, c.z };
    const T pd[3] = { d.x, d.y, d.z };
    const double approx = adaptive_predicate::orient3d(pa, pb, pc, pd);
    if(approx > 0.0) return +1;
    if(approx < 0.0) return -1;
    const ExactPlane3 plane = ExactPlane3::from_points(ExactPoint3::from_vec3(a),
                                                       ExactPoint3::from_vec3(b),
                                                       ExactPoint3::from_vec3(c));
    return (-plane.eval(ExactPoint3::from_vec3(d))).sign();
}

// Supported input contract for from_fv_surface_mesh:
//
// The converter accepts finite, closed, consistently oriented triangular or
// triangulatable surface meshes. The intended exact-solid contract is stricter:
// the mesh must be a valid, topologically well-defined b-rep solid with no
// self-intersections, no non-manifold geometric overlaps, no zero-area faces
// after triangulation, no contradictory coincident shells, and a bounded solid
// under the mesh orientation convention. Detection of every clause is staged by
// the repair plan; each implemented detection path must throw instead of
// silently returning an empty or placeholder volume.

template <class T>
int classify_point(const bsp_plane<T> &P, const vec3<T> &v) {
    const T pa[3] = { P.anchors[0].x, P.anchors[0].y, P.anchors[0].z };
    const T pb[3] = { P.anchors[1].x, P.anchors[1].y, P.anchors[1].z };
    const T pc[3] = { P.anchors[2].x, P.anchors[2].y, P.anchors[2].z };
    const T pd[3] = { v.x, v.y, v.z };
    const double s = adaptive_predicate::orient3d(pa, pb, pc, pd);
    return (s > 0.0) ? +1 : ((s < 0.0) ? -1 : 0);
}

template <class T>
bsp_plane<T> plane_from_triangle(const vec3<T> &a,
                                 const vec3<T> &b,
                                 const vec3<T> &c) {
    return bsp_plane<T>(a, b, c);
}

// ---- BSP Tree: clone and complement ----

template <class T, class I>
using NodePtr = std::unique_ptr<typename bsp_tree_volume<T, I>::Node>;
template <class T, class I>
using NodeType = typename bsp_tree_volume<T, I>::NodeType;

template <class T, class I>
NodePtr<T, I> make_out_node() {
    return std::make_unique<typename bsp_tree_volume<T, I>::Node>(NodeType<T, I>::Out);
}

template <class T, class I>
NodePtr<T, I> make_in_node() {
    return std::make_unique<typename bsp_tree_volume<T, I>::Node>(NodeType<T, I>::In);
}

template <class T, class I>
NodePtr<T, I> explicit_out_if_null(NodePtr<T, I> node) {
    return node ? std::move(node) : make_out_node<T, I>();
}

template <class T, class I>
NodePtr<T, I> clone_node(const typename bsp_tree_volume<T, I>::Node *n) {
    if(!n) return nullptr;
    return NodePtr<T, I>(n->clone());
}

template <class T, class I>
NodePtr<T, I> clone_or_out(const typename bsp_tree_volume<T, I>::Node *n) {
    auto cloned = clone_node<T, I>(n);
    return explicit_out_if_null<T, I>(std::move(cloned));
}

template <class T, class I>
bool same_explicit_leaf(const NodePtr<T, I> &a, const NodePtr<T, I> &b) {
    using NT = NodeType<T, I>;
    return a && b
        && a->type != NT::Partition
        && b->type != NT::Partition
        && a->type == b->type;
}

template <class T, class I>
NodePtr<T, I> make_partition_or_collapse(const bsp_plane<T> &plane,
                                         NodePtr<T, I> front,
                                         NodePtr<T, I> back) {
    using Node = typename bsp_tree_volume<T, I>::Node;
    front = explicit_out_if_null<T, I>(std::move(front));
    back = explicit_out_if_null<T, I>(std::move(back));
    if(same_explicit_leaf<T, I>(front, back)) {
        return front;
    }
    return std::make_unique<Node>(plane, std::move(front), std::move(back));
}

CanonicalPlaneKey negated_plane_key(CanonicalPlaneKey key) {
    for(auto &c : key.coeffs) c = -c;
    return key;
}

template <class T>
int exact_coplanar_orientation_relation(const bsp_plane<T> &p,
                                        const bsp_plane<T> &q) {
    const CanonicalPlaneKey pk = canonical_plane_key(p);
    const CanonicalPlaneKey qk = canonical_plane_key(q);
    if(pk.coeffs == qk.coeffs) return +1;
    if(pk.coeffs == negated_plane_key(qk).coeffs) return -1;
    return 0;
}

template <class T, class I>
NodePtr<T, I> complement_tree(NodePtr<T, I> node) {
    if(!node) return make_in_node<T, I>();
    if(node->type == NodeType<T, I>::In)
        return make_out_node<T, I>();
    if(node->type == NodeType<T, I>::Out)
        return make_in_node<T, I>();

    auto fc = complement_tree<T, I>(std::move(node->front));
    auto bc = complement_tree<T, I>(std::move(node->back));

    return make_partition_or_collapse<T, I>(node->partition_plane,
                                            std::move(fc),
                                            std::move(bc));
}

// Partition a BSP tree by a plane.
// Returns (front_fragment, back_fragment).
//
// When the tree being partitioned contains a plane coplanar with the
// partition plane, that coplanar plane is collapsed (flattened) so it
// does not appear duplicated in the output.  Its subtrees are placed
// directly into the appropriate half-space fragments.
template <class T, class I>
std::pair<NodePtr<T, I>, NodePtr<T, I>>
partition_tree(const bsp_plane<T> &P, NodePtr<T, I> node) {
    using Node = typename bsp_tree_volume<T, I>::Node;
    using NT = NodeType<T, I>;

    if(!node) {
        return {make_out_node<T, I>(), make_out_node<T, I>()};
    }
    if(node->type != NT::Partition) {
        auto nt = node->type;
        return {std::make_unique<Node>(nt), std::make_unique<Node>(nt)};
    }

    const bsp_plane<T> &Q = node->partition_plane;

    const int coplanar_relation = exact_coplanar_orientation_relation(P, Q);
    if(coplanar_relation != 0) {
        NodePtr<T, I> front_result;
        NodePtr<T, I> back_result;
        if(coplanar_relation > 0) {
            front_result = std::move(node->front);
            back_result = std::move(node->back);
        } else {
            front_result = std::move(node->back);
            back_result = std::move(node->front);
        }
        return {explicit_out_if_null<T, I>(std::move(front_result)),
                explicit_out_if_null<T, I>(std::move(back_result))};
    }

    auto [ff, fb] = partition_tree<T, I>(P, std::move(node->front));
    auto [bf, bb] = partition_tree<T, I>(P, std::move(node->back));

    NodePtr<T, I> front_result = make_partition_or_collapse<T, I>(Q, std::move(ff), std::move(bf));
    NodePtr<T, I> back_result = make_partition_or_collapse<T, I>(Q, std::move(fb), std::move(bb));

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

template <class T, class I>
NodePtr<T, I> canonicalize_empty_bounded_result(NodePtr<T, I> node);

// Merge two BSP trees with a boolean operation.
// op: 0 = union, 1 = intersection, 2 = subtraction (A-B)
template <class T, class I>
NodePtr<T, I> merge_bsp(NodePtr<T, I> A, NodePtr<T, I> B, int op) {
    using NT = NodeType<T, I>;

    // Handle leaf cases for A.
    if(!A || A->type != NT::Partition) {
        const bool a_in = (A && A->type == NT::In);
        switch(op) {
            case 0:
                return a_in ? make_in_node<T, I>()
                            : clone_or_out<T, I>(B.get());
            case 1:
                return a_in ? clone_or_out<T, I>(B.get())
                            : make_out_node<T, I>();
            case 2:
                return a_in ? complement_tree<T, I>(std::move(B))
                            : make_out_node<T, I>();
        }
    }

    // Handle leaf cases for B.
    if(!B || B->type != NT::Partition) {
        const bool b_in = (B && B->type == NT::In);
        switch(op) {
            case 0:
                return b_in ? make_in_node<T, I>()
                            : clone_or_out<T, I>(A.get());
            case 1:
                return b_in ? clone_or_out<T, I>(A.get())
                            : make_out_node<T, I>();
            case 2:
                return b_in ? make_out_node<T, I>()
                            : clone_or_out<T, I>(A.get());
        }
    }

    // Both are partition nodes. Use A's plane to partition B.
    const bsp_plane<T> &P = A->partition_plane;
    auto [B_front, B_back] = partition_tree<T, I>(P, std::move(B));

    auto new_front = merge_bsp<T, I>(clone_node<T, I>(A->front.get()),
                                     std::move(B_front), op);
    auto new_back  = merge_bsp<T, I>(clone_node<T, I>(A->back.get()),
                                     std::move(B_back), op);

    return make_partition_or_collapse<T, I>(P, std::move(new_front), std::move(new_back));
}


// ---- Mesh -> BSP conversion helpers ----

template <class T>
struct TriangleRec {
    std::array<vec3<T>, 3> v;
    bsp_plane<T> pl;
    ExactPlane3 exact_plane;
    size_t original_face_id = 0;
};

struct PlanarArrangementVertex {
    ExactPoint2 p;
    ExactVertexSymbol symbol;
};

struct PlanarArrangementHalfEdge {
    size_t source = 0;
    size_t target = 0;
    size_t twin = 0;
    size_t next = 0;
    size_t prev = 0;
    size_t face = std::numeric_limits<size_t>::max();
    std::set<size_t> source_face_ids;
};

struct PlanarArrangementFace {
    std::vector<size_t> half_edges;
    bool bounded = false;
    int material_side = 0;
};

struct PlanarArrangement {
    std::vector<PlanarArrangementVertex> vertices;
    std::vector<PlanarArrangementHalfEdge> half_edges;
    std::vector<PlanarArrangementFace> faces;
};

enum class ArrangementCellClass : uint8_t {
    Unknown = 0,
    Out     = 1,
    In      = 2
};

struct Arrangement3DCell {
    std::vector<int8_t> plane_signs;
    ArrangementCellClass classification = ArrangementCellClass::Unknown;
    std::set<size_t> adjacent_cells;
};

struct Arrangement3DSurfacePatch {
    size_t plane_id = 0;
    size_t source_face_id = 0;
    size_t front_cell = 0;
    size_t back_cell = 0;
};

struct GlobalCellArrangement3D {
    std::vector<ExactPlane3> planes;
    std::vector<CanonicalPlaneKey> plane_keys;
    std::vector<Arrangement3DCell> cells;
    std::vector<Arrangement3DSurfacePatch> patches;
};

struct PlanarSegment {
    ExactPoint2 a;
    ExactPoint2 b;
    size_t source_face_id = 0;
};

ExactPoint3 triangle_centroid_exact(const ExactPoint3 &a,
                                    const ExactPoint3 &b,
                                    const ExactPoint3 &c) {
    const ExactScalar third = ExactScalar(1) / ExactScalar(3);
    return (a + b + c) * third;
}

ExactPoint3 plane_normal_exact(const ExactPlane3 &plane) {
    return {plane.a, plane.b, plane.c};
}

template <class T>
T exact_scalar_to_float(const ExactScalar &s) {
    const long double n = s.n.convert_to<long double>();
    const long double d = s.d.convert_to<long double>();
    return static_cast<T>(n / d);
}

template <class T>
vec3<T> exact_point_to_vec3(const ExactPoint3 &p) {
    return vec3<T>(exact_scalar_to_float<T>(p.x),
                   exact_scalar_to_float<T>(p.y),
                   exact_scalar_to_float<T>(p.z));
}

template <class T>
bsp_plane<T> bsp_plane_from_exact_plane(const ExactPlane3 &plane) {
    const ExactScalar ax = (plane.a.sign() < 0) ? -plane.a : plane.a;
    const ExactScalar ay = (plane.b.sign() < 0) ? -plane.b : plane.b;
    const ExactScalar az = (plane.c.sign() < 0) ? -plane.c : plane.c;

    ExactPoint3 p0;
    ExactPoint3 p1;
    ExactPoint3 p2;
    if(!(az < ax) && !(az < ay)) {
        p0 = {ExactScalar(0), ExactScalar(0), -plane.d / plane.c};
        p1 = {ExactScalar(1), ExactScalar(0), -(plane.d + plane.a) / plane.c};
        p2 = {ExactScalar(0), ExactScalar(1), -(plane.d + plane.b) / plane.c};
    } else if(!(ay < ax)) {
        p0 = {ExactScalar(0), -plane.d / plane.b, ExactScalar(0)};
        p1 = {ExactScalar(1), -(plane.d + plane.a) / plane.b, ExactScalar(0)};
        p2 = {ExactScalar(0), -(plane.d + plane.c) / plane.b, ExactScalar(1)};
    } else {
        p0 = {-plane.d / plane.a, ExactScalar(0), ExactScalar(0)};
        p1 = {-(plane.d + plane.b) / plane.a, ExactScalar(1), ExactScalar(0)};
        p2 = {-(plane.d + plane.c) / plane.a, ExactScalar(0), ExactScalar(1)};
    }

    const ExactPoint3 generated_normal = cross_exact(p1 - p0, p2 - p0);
    if(dot_exact(generated_normal, plane_normal_exact(plane)).sign() < 0) {
        std::swap(p1, p2);
    }

    return bsp_plane<T>(exact_point_to_vec3<T>(p0),
                        exact_point_to_vec3<T>(p1),
                        exact_point_to_vec3<T>(p2));
}

ExactPoint2 project_to_support_plane_2d(const ExactPoint3 &p,
                                        const ExactPlane3 &plane) {
    const ExactScalar ax = (plane.a.sign() < 0) ? -plane.a : plane.a;
    const ExactScalar ay = (plane.b.sign() < 0) ? -plane.b : plane.b;
    const ExactScalar az = (plane.c.sign() < 0) ? -plane.c : plane.c;
    if(!(ax < ay) && !(ax < az)) return {p.y, p.z};
    if(!(ay < az)) return {p.x, p.z};
    return {p.x, p.y};
}

bool point_on_segment_2d(const ExactPoint2 &p,
                         const ExactPoint2 &a,
                         const ExactPoint2 &b) {
    if(!orient_exact_2d(a, b, p).is_zero()) return false;
    return std::min(a.x, b.x) <= p.x && p.x <= std::max(a.x, b.x)
        && std::min(a.y, b.y) <= p.y && p.y <= std::max(a.y, b.y);
}

void add_segment_intersections(const PlanarSegment &s0,
                               const PlanarSegment &s1,
                               std::vector<ExactPoint2> &cuts0,
                               std::vector<ExactPoint2> &cuts1) {
    const ExactPoint2 r = s0.b - s0.a;
    const ExactPoint2 s = s1.b - s1.a;
    const ExactScalar denom = cross_exact_2d(r, s);
    const ExactPoint2 qmp = s1.a - s0.a;

    if(!denom.is_zero()) {
        const ExactScalar t = cross_exact_2d(qmp, s) / denom;
        const ExactScalar u = cross_exact_2d(qmp, r) / denom;
        if(ExactScalar(0) <= t && t <= ExactScalar(1)
        && ExactScalar(0) <= u && u <= ExactScalar(1)) {
            const ExactPoint2 hit = s0.a + r * t;
            cuts0.push_back(hit);
            cuts1.push_back(hit);
        }
        return;
    }

    if(!cross_exact_2d(qmp, r).is_zero()) return;

    const std::array<ExactPoint2, 4> candidates = {{s0.a, s0.b, s1.a, s1.b}};
    for(const auto &p : candidates) {
        const bool on0 = point_on_segment_2d(p, s0.a, s0.b);
        const bool on1 = point_on_segment_2d(p, s1.a, s1.b);
        if(on0 && on1) {
            cuts0.push_back(p);
            cuts1.push_back(p);
        }
    }
}

bool direction_angle_less(const ExactPoint2 &a, const ExactPoint2 &b) {
    const auto upper = [](const ExactPoint2 &v) {
        return (v.y.sign() > 0) || (v.y.is_zero() && v.x.sign() >= 0);
    };
    const bool au = upper(a);
    const bool bu = upper(b);
    if(au != bu) return au > bu;
    const int c = cross_exact_2d(a, b).sign();
    if(c != 0) return c > 0;
    return std::tie(a.x, a.y) < std::tie(b.x, b.y);
}

ExactScalar signed_face_area2(const PlanarArrangement &arr,
                              const std::vector<size_t> &half_edges) {
    ExactScalar area2(0);
    for(const size_t he_id : half_edges) {
        const auto &he = arr.half_edges[he_id];
        const ExactPoint2 &a = arr.vertices[he.source].p;
        const ExactPoint2 &b = arr.vertices[he.target].p;
        area2 = area2 + cross_exact_2d(a, b);
    }
    return area2;
}

PlanarArrangement build_planar_arrangement(const std::vector<PlanarSegment> &segments) {
    PlanarArrangement arr;
    if(segments.empty()) return arr;

    std::vector<std::vector<ExactPoint2>> cuts(segments.size());
    for(size_t i = 0; i < segments.size(); ++i) {
        cuts[i].push_back(segments[i].a);
        cuts[i].push_back(segments[i].b);
    }
    for(size_t i = 0; i < segments.size(); ++i) {
        for(size_t j = i + 1; j < segments.size(); ++j) {
            add_segment_intersections(segments[i], segments[j], cuts[i], cuts[j]);
        }
    }

    std::map<ExactPoint2, size_t, decltype(&exact_point2_less)> vertex_ids(&exact_point2_less);
    const auto vertex_id = [&](const ExactPoint2 &p,
                               PlanarArrangement &a) -> size_t {
        auto it = vertex_ids.find(p);
        if(it != vertex_ids.end()) return it->second;
        const size_t id = a.vertices.size();
        a.vertices.push_back({p, {ExactVertexKind::Coordinate, {{id, 0, 0}}}});
        vertex_ids.emplace(p, id);
        return id;
    };

    std::map<std::pair<size_t, size_t>, std::set<size_t>> directed_sources;
    for(size_t si = 0; si < segments.size(); ++si) {
        auto &pts = cuts[si];
        std::sort(pts.begin(), pts.end(), exact_point2_less);
        pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
        if(!exact_point2_less(segments[si].a, segments[si].b)) {
            std::reverse(pts.begin(), pts.end());
        }
        for(size_t pi = 0; pi + 1 < pts.size(); ++pi) {
            if(pts[pi] == pts[pi + 1]) continue;
            const size_t a = vertex_id(pts[pi], arr);
            const size_t b = vertex_id(pts[pi + 1], arr);
            if(a == b) continue;
            directed_sources[{a, b}].insert(segments[si].source_face_id);
            directed_sources[{b, a}].insert(segments[si].source_face_id);
        }
    }

    std::map<std::pair<size_t, size_t>, size_t> half_edge_ids;
    for(const auto &entry : directed_sources) {
        const size_t id = arr.half_edges.size();
        half_edge_ids.emplace(entry.first, id);
        PlanarArrangementHalfEdge he;
        he.source = entry.first.first;
        he.target = entry.first.second;
        he.source_face_ids = entry.second;
        arr.half_edges.push_back(std::move(he));
    }
    for(size_t he_id = 0; he_id < arr.half_edges.size(); ++he_id) {
        const auto &he = arr.half_edges[he_id];
        arr.half_edges[he_id].twin = half_edge_ids.at({he.target, he.source});
    }

    std::vector<std::vector<size_t>> outgoing(arr.vertices.size());
    for(size_t he_id = 0; he_id < arr.half_edges.size(); ++he_id) {
        outgoing[arr.half_edges[he_id].source].push_back(he_id);
    }
    for(auto &list : outgoing) {
        std::sort(list.begin(), list.end(), [&](size_t lhs, size_t rhs) {
            const auto &l = arr.half_edges[lhs];
            const auto &r = arr.half_edges[rhs];
            const ExactPoint2 dl = arr.vertices[l.target].p - arr.vertices[l.source].p;
            const ExactPoint2 dr = arr.vertices[r.target].p - arr.vertices[r.source].p;
            return direction_angle_less(dl, dr);
        });
    }

    for(size_t he_id = 0; he_id < arr.half_edges.size(); ++he_id) {
        const size_t twin = arr.half_edges[he_id].twin;
        const size_t v = arr.half_edges[he_id].target;
        const auto &list = outgoing[v];
        const auto it = std::find(list.begin(), list.end(), twin);
        if(it == list.end()) throw std::runtime_error("build_planar_arrangement: missing twin in outgoing list.");
        const size_t pos = static_cast<size_t>(std::distance(list.begin(), it));
        const size_t next_pos = (pos == 0) ? (list.size() - 1) : (pos - 1);
        arr.half_edges[he_id].next = list[next_pos];
        arr.half_edges[list[next_pos]].prev = he_id;
    }

    for(size_t start = 0; start < arr.half_edges.size(); ++start) {
        if(arr.half_edges[start].face != std::numeric_limits<size_t>::max()) continue;
        const size_t face_id = arr.faces.size();
        PlanarArrangementFace face;
        size_t cur = start;
        do {
            if(arr.half_edges[cur].face != std::numeric_limits<size_t>::max()) {
                throw std::runtime_error("build_planar_arrangement: non-manifold half-edge face walk.");
            }
            arr.half_edges[cur].face = face_id;
            face.half_edges.push_back(cur);
            cur = arr.half_edges[cur].next;
        } while(cur != start);

        const ExactScalar area2 = signed_face_area2(arr, face.half_edges);
        face.bounded = area2.sign() > 0;
        if(face.bounded) face.material_side = 1;
        arr.faces.push_back(std::move(face));
    }

    return arr;
}

template <class T>
std::vector<PlanarArrangement> build_planar_arrangements_for_support_planes(
    const std::vector<TriangleRec<T>> &tris) {
    std::map<CanonicalPlaneKey, std::vector<PlanarSegment>> plane_segments;
    for(const auto &tri : tris) {
        auto &segments = plane_segments[canonical_plane_key(tri.exact_plane)];
        const std::array<ExactPoint2, 3> p = {{
            project_to_support_plane_2d(ExactPoint3::from_vec3(tri.v[0]), tri.exact_plane),
            project_to_support_plane_2d(ExactPoint3::from_vec3(tri.v[1]), tri.exact_plane),
            project_to_support_plane_2d(ExactPoint3::from_vec3(tri.v[2]), tri.exact_plane)
        }};
        segments.push_back({p[0], p[1], tri.original_face_id});
        segments.push_back({p[1], p[2], tri.original_face_id});
        segments.push_back({p[2], p[0], tri.original_face_id});
    }

    std::vector<PlanarArrangement> arrangements;
    arrangements.reserve(plane_segments.size());
    for(const auto &entry : plane_segments) {
        arrangements.push_back(build_planar_arrangement(entry.second));
    }
    return arrangements;
}

template <class T>
GlobalCellArrangement3D build_global_cell_arrangement(
    const std::vector<TriangleRec<T>> &tris,
    const std::vector<PlanarArrangement> &planar_arrangements) {
    if(tris.empty()) {
        throw std::invalid_argument("build_global_cell_arrangement: no input triangles.");
    }

    GlobalCellArrangement3D arrangement;
    std::map<CanonicalPlaneKey, size_t> plane_ids;
    for(const auto &tri : tris) {
        const CanonicalPlaneKey key = canonical_plane_key(tri.exact_plane);
        if(plane_ids.find(key) != plane_ids.end()) continue;
        const size_t plane_id = arrangement.planes.size();
        plane_ids.emplace(key, plane_id);
        arrangement.planes.push_back(tri.exact_plane);
        arrangement.plane_keys.push_back(key);
    }

    if(arrangement.planes.size() != planar_arrangements.size()) {
        throw std::runtime_error("build_global_cell_arrangement: support plane/planar arrangement count mismatch.");
    }

    using CellKey = std::pair<std::vector<int8_t>, ArrangementCellClass>;
    std::map<CellKey, size_t> cell_ids;
    const auto cell_id_for = [&](const std::vector<int8_t> &signs,
                                 ArrangementCellClass classification,
                                 GlobalCellArrangement3D &a) -> size_t {
        const CellKey key{signs, classification};
        const auto it = cell_ids.find(key);
        if(it != cell_ids.end()) return it->second;
        const size_t id = a.cells.size();
        Arrangement3DCell cell;
        cell.plane_signs = signs;
        cell.classification = classification;
        a.cells.push_back(std::move(cell));
        cell_ids.emplace(key, id);
        return id;
    };

    const auto classify_near_patch = [&](const ExactPoint3 &p,
                                         const ExactPoint3 &offset,
                                         size_t on_plane) {
        std::vector<int8_t> signs(arrangement.planes.size(), 0);
        const ExactPoint3 q = p + offset;
        for(size_t pi = 0; pi < arrangement.planes.size(); ++pi) {
            const int s = arrangement.planes[pi].eval(q).sign();
            if(s == 0) {
                const int offset_sign = arrangement.planes[pi].eval(offset).sign();
                if(pi == on_plane || offset_sign != 0) {
                    signs[pi] = (offset_sign >= 0) ? int8_t(+1) : int8_t(-1);
                } else {
                    signs[pi] = int8_t(+1);
                }
            } else {
                signs[pi] = static_cast<int8_t>(s);
            }
        }
        return signs;
    };

    for(const auto &tri : tris) {
        const size_t plane_id = plane_ids.at(canonical_plane_key(tri.exact_plane));
        const ExactPoint3 a = ExactPoint3::from_vec3(tri.v[0]);
        const ExactPoint3 b = ExactPoint3::from_vec3(tri.v[1]);
        const ExactPoint3 c = ExactPoint3::from_vec3(tri.v[2]);
        const ExactPoint3 centroid = triangle_centroid_exact(a, b, c);
        const ExactPoint3 normal = plane_normal_exact(tri.exact_plane);

        const size_t front_cell = cell_id_for(classify_near_patch(centroid, normal, plane_id),
                                             ArrangementCellClass::Out,
                                             arrangement);
        const size_t back_cell = cell_id_for(classify_near_patch(centroid, normal * ExactScalar(-1), plane_id),
                                            ArrangementCellClass::In,
                                            arrangement);
        arrangement.cells[front_cell].adjacent_cells.insert(back_cell);
        arrangement.cells[back_cell].adjacent_cells.insert(front_cell);

        arrangement.patches.push_back({plane_id, tri.original_face_id, front_cell, back_cell});
    }

    for(size_t patch_id = 0; patch_id < arrangement.patches.size(); ++patch_id) {
        const auto &patch = arrangement.patches[patch_id];
        if(patch.front_cell >= arrangement.cells.size() || patch.back_cell >= arrangement.cells.size()) {
            std::ostringstream os;
            os << "build_global_cell_arrangement: patch " << patch_id << " has invalid adjacent cell ids.";
            throw std::runtime_error(os.str());
        }
        if(arrangement.cells[patch.front_cell].classification != ArrangementCellClass::Out
        || arrangement.cells[patch.back_cell].classification != ArrangementCellClass::In) {
            std::ostringstream os;
            os << "build_global_cell_arrangement: contradictory material side at source face "
               << patch.source_face_id << '.';
            throw std::runtime_error(os.str());
        }
    }

    return arrangement;
}

GlobalCellArrangement3D canonicalize_cells_for_bsp(const GlobalCellArrangement3D &arrangement) {
    GlobalCellArrangement3D result;
    result.planes = arrangement.planes;
    result.plane_keys = arrangement.plane_keys;

    if(result.planes.empty()) return result;

    Arrangement3DCell inside;
    inside.plane_signs.assign(result.planes.size(), int8_t(-1));
    inside.classification = ArrangementCellClass::In;
    result.cells.push_back(std::move(inside));

    for(size_t plane_id = 0; plane_id < result.planes.size(); ++plane_id) {
        Arrangement3DCell outside;
        outside.plane_signs.assign(result.planes.size(), int8_t(-1));
        outside.plane_signs[plane_id] = int8_t(+1);
        outside.classification = ArrangementCellClass::Out;
        result.cells.push_back(std::move(outside));
    }

    return result;
}

bool operator<(const CanonicalPlaneKey &a, const CanonicalPlaneKey &b) {
    return a.coeffs < b.coeffs;
}

CanonicalPlaneKey canonical_plane_key(const ExactPlane3 &plane) {
    const std::array<ExactScalar, 4> c = {{plane.a, plane.b, plane.c, plane.d}};
    ExactInt lcm = 1;
    for(const auto &s : c) {
        const ExactInt g = gcd_exact_int(lcm, s.d);
        lcm = (lcm / g) * s.d;
    }

    CanonicalPlaneKey key;
    ExactInt common = 0;
    for(size_t i = 0; i < c.size(); ++i) {
        key.coeffs[i] = c[i].n * (lcm / c[i].d);
        common = (i == 0) ? abs_exact_int(key.coeffs[i]) : gcd_exact_int(common, key.coeffs[i]);
    }
    if(common != 0) {
        for(auto &v : key.coeffs) v /= common;
    }
    return key;
}

template <class T>
CanonicalPlaneKey canonical_plane_key(const bsp_plane<T> &plane) {
    return canonical_plane_key(ExactPlane3::from_points(ExactPoint3::from_vec3(plane.anchors[0]),
                                                        ExactPoint3::from_vec3(plane.anchors[1]),
                                                        ExactPoint3::from_vec3(plane.anchors[2])));
}

template <class T, class I>
struct CanonicalTriangleRec {
    std::array<I, 3> idx;
    std::array<ExactPoint3, 3> exact;
    size_t original_face_id = 0;
};

template <class T, class I>
void reverse_all_faces(fv_surface_mesh<T, I> &mesh) {
    for(auto &f : mesh.faces) {
        std::reverse(f.begin(), f.end());
    }
}

template <class T, class I>
int exact_signed_volume_sign(const fv_surface_mesh<T, I> &mesh) {
    ExactScalar sum(0);
    for(const auto &f : mesh.faces) {
        if(f.size() != 3) continue;
        const ExactPoint3 a = ExactPoint3::from_vec3(mesh.vertices.at(f[0]));
        const ExactPoint3 b = ExactPoint3::from_vec3(mesh.vertices.at(f[1]));
        const ExactPoint3 c = ExactPoint3::from_vec3(mesh.vertices.at(f[2]));
        sum = sum + dot_exact(a, cross_exact(b, c));
    }
    return sum.sign();
}

template <class I>
std::array<I, 3> sorted_face_key(std::array<I, 3> f) {
    std::sort(f.begin(), f.end());
    return f;
}

template <class I>
bool same_oriented_cycle(const std::array<I, 3> &a, const std::array<I, 3> &b) {
    for(size_t shift = 0; shift < 3; ++shift) {
        if(a[0] == b[shift] && a[1] == b[(shift + 1) % 3] && a[2] == b[(shift + 2) % 3]) {
            return true;
        }
    }
    return false;
}

template <class I>
std::string face_id_message(const char *what, size_t face_id) {
    std::ostringstream os;
    os << "bsp_tree_volume::from_fv_surface_mesh: " << what << " at face " << face_id << '.';
    return os.str();
}

template <class I>
std::string face_indices_message(const char *what, size_t face_id, const std::vector<I> &face) {
    std::ostringstream os;
    os << "bsp_tree_volume::from_fv_surface_mesh: " << what << " at face " << face_id << " with indices [";
    for(size_t i = 0; i < face.size(); ++i) {
        if(i != 0) os << ',';
        os << face[i];
    }
    os << "].";
    return os.str();
}

template <class I>
std::string edge_message(const char *what, I a, I b, size_t count) {
    std::ostringstream os;
    os << "bsp_tree_volume::from_fv_surface_mesh: " << what
       << " at canonical edge (" << a << ',' << b << ") with count " << count << '.';
    return os.str();
}

template <class T, class I>
void canonicalize_mesh_for_bsp(fv_surface_mesh<T, I> &mesh,
                               std::vector<CanonicalTriangleRec<T, I>> &triangles) {
    std::map<ExactPoint3, I, decltype(&exact_point_less)> vertex_map(&exact_point_less);
    std::vector<vec3<T>> canonical_vertices;
    std::vector<I> remap(mesh.vertices.size());

    for(size_t vi = 0; vi < mesh.vertices.size(); ++vi) {
        const ExactPoint3 ep = ExactPoint3::from_vec3(mesh.vertices[vi]);
        auto it = vertex_map.find(ep);
        if(it == vertex_map.end()) {
            const I new_id = static_cast<I>(canonical_vertices.size());
            canonical_vertices.push_back(mesh.vertices[vi]);
            vertex_map.emplace(ep, new_id);
            remap[vi] = new_id;
        } else {
            remap[vi] = it->second;
        }
    }

    std::map<std::array<I, 3>, std::array<I, 3>> seen_faces;
    std::vector<std::vector<I>> canonical_faces;
    triangles.clear();
    triangles.reserve(mesh.faces.size());

    for(size_t fi = 0; fi < mesh.faces.size(); ++fi) {
        const auto &f = mesh.faces[fi];
        if(f.size() != 3) {
            throw std::invalid_argument(face_indices_message<I>("non-triangular face after triangulation", fi, f));
        }
        if(f[0] >= mesh.vertices.size() || f[1] >= mesh.vertices.size() || f[2] >= mesh.vertices.size()) {
            throw std::invalid_argument(face_indices_message<I>("out-of-range face index", fi, f));
        }

        const std::array<I, 3> idx = {{remap[f[0]], remap[f[1]], remap[f[2]]}};
        if(idx[0] == idx[1] || idx[1] == idx[2] || idx[2] == idx[0]) {
            throw std::invalid_argument(face_id_message<I>("zero-area face after exact vertex canonicalization", fi));
        }

        const std::array<ExactPoint3, 3> ep = {{
            ExactPoint3::from_vec3(canonical_vertices[idx[0]]),
            ExactPoint3::from_vec3(canonical_vertices[idx[1]]),
            ExactPoint3::from_vec3(canonical_vertices[idx[2]])
        }};
        const ExactPoint3 normal = cross_exact(ep[1] - ep[0], ep[2] - ep[0]);
        if(normal.x.is_zero() && normal.y.is_zero() && normal.z.is_zero()) {
            throw std::invalid_argument(face_id_message<I>("zero-area face under exact arithmetic", fi));
        }

        const auto key = sorted_face_key(idx);
        const auto seen = seen_faces.find(key);
        if(seen != seen_faces.end()) {
            if(same_oriented_cycle(seen->second, idx)) {
                continue;
            }
            throw std::invalid_argument(face_id_message<I>("opposite-orientation duplicate face", fi));
        }
        seen_faces.emplace(key, idx);

        canonical_faces.push_back({idx[0], idx[1], idx[2]});
        triangles.push_back({idx, ep, fi});
    }

    if(canonical_faces.empty()) {
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh contains no non-duplicate triangular faces.");
    }

    std::map<std::pair<I, I>, size_t> undirected_edges;
    std::map<std::pair<I, I>, size_t> directed_edges;
    for(size_t fi = 0; fi < canonical_faces.size(); ++fi) {
        const auto &f = canonical_faces[fi];
        for(size_t ei = 0; ei < 3; ++ei) {
            const I a = f[ei];
            const I b = f[(ei + 1) % 3];
            const auto undirected = std::minmax(a, b);
            ++undirected_edges[undirected];
            ++directed_edges[{a, b}];
        }
    }
    for(const auto &entry : undirected_edges) {
        if(entry.second != 2) {
            throw std::invalid_argument(edge_message<I>("canonicalized mesh is not a closed edge manifold",
                                                        entry.first.first,
                                                        entry.first.second,
                                                        entry.second));
        }
    }
    for(const auto &entry : directed_edges) {
        const auto reverse = std::make_pair(entry.first.second, entry.first.first);
        const auto rit = directed_edges.find(reverse);
        if(rit == directed_edges.end() || rit->second != entry.second) {
            const size_t reverse_count = (rit == directed_edges.end()) ? size_t(0) : rit->second;
            std::ostringstream os;
            os << "bsp_tree_volume::from_fv_surface_mesh: canonicalized mesh has inconsistent edge orientation"
               << " at directed canonical edge (" << entry.first.first << ',' << entry.first.second << ")"
               << " with forward count " << entry.second
               << " and reverse count " << reverse_count << '.';
            throw std::invalid_argument(os.str());
        }
    }

    mesh.vertices = std::move(canonical_vertices);
    mesh.faces = std::move(canonical_faces);

    const int volume_sign = exact_signed_volume_sign(mesh);
    if(volume_sign < 0) {
        reverse_all_faces(mesh);
        for(auto &tri : triangles) {
            std::swap(tri.idx[1], tri.idx[2]);
            std::swap(tri.exact[1], tri.exact[2]);
        }
    } else if(volume_sign == 0) {
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: canonicalized mesh has ambiguous zero signed volume.");
    }
}

template <class T, class I>
NodePtr<T, I> build_bsp_from_classified_cells(
    const GlobalCellArrangement3D &arrangement,
    std::vector<size_t> cell_ids,
    std::set<size_t> remaining_planes,
    size_t depth) {
    using Node = typename bsp_tree_volume<T, I>::Node;
    using NT = NodeType<T, I>;

    if(cell_ids.empty()) {
        return std::make_unique<Node>(NT::Out);
    }
    if(depth > arrangement.planes.size()) {
        std::ostringstream os;
        os << "bsp_tree_volume::from_fv_surface_mesh: classified-cell BSP construction exceeded depth "
           << arrangement.planes.size() << " with " << cell_ids.size() << " active cells.";
        throw std::runtime_error(os.str());
    }

    const auto first_class = arrangement.cells.at(cell_ids.front()).classification;
    bool uniform = (first_class == ArrangementCellClass::In || first_class == ArrangementCellClass::Out);
    for(const size_t cell_id : cell_ids) {
        const auto cls = arrangement.cells.at(cell_id).classification;
        if(cls == ArrangementCellClass::Unknown) {
            std::ostringstream os;
            os << "bsp_tree_volume::from_fv_surface_mesh: arrangement cell " << cell_id
               << " reached BSP construction without classification.";
            throw std::runtime_error(os.str());
        }
        if(cls != first_class) uniform = false;
    }
    if(uniform) {
        return std::make_unique<Node>(first_class == ArrangementCellClass::In ? NT::In : NT::Out);
    }

    bool have_choice = false;
    size_t best_plane = 0;
    size_t best_balance = 0;
    for(const size_t plane_id : remaining_planes) {
        size_t front_count = 0;
        size_t back_count = 0;
        for(const size_t cell_id : cell_ids) {
            const auto &cell = arrangement.cells.at(cell_id);
            if(plane_id >= cell.plane_signs.size()) {
                std::ostringstream os;
                os << "bsp_tree_volume::from_fv_surface_mesh: arrangement cell " << cell_id
                   << " sign vector has " << cell.plane_signs.size()
                   << " entries but support plane " << plane_id << " was requested.";
                throw std::runtime_error(os.str());
            }
            if(cell.plane_signs[plane_id] >= 0) ++front_count;
            if(cell.plane_signs[plane_id] <= 0) ++back_count;
        }
        if(front_count == 0 || back_count == 0) continue;

        const size_t balance = (front_count > back_count) ? (front_count - back_count)
                                                          : (back_count - front_count);
        if(!have_choice || std::tie(balance, plane_id) < std::tie(best_balance, best_plane)) {
            have_choice = true;
            best_plane = plane_id;
            best_balance = balance;
        }
    }

    if(!have_choice) {
        std::ostringstream os;
        os << "bsp_tree_volume::from_fv_surface_mesh: classified-cell BSP construction found "
           << cell_ids.size() << " mixed cells that no remaining support plane separates at depth "
           << depth << "; active cells [";
        for(size_t i = 0; i < cell_ids.size(); ++i) {
            if(i != 0) os << ',';
            os << cell_ids[i];
        }
        os << "] and remaining planes [";
        bool first = true;
        for(const size_t plane_id : remaining_planes) {
            if(!first) os << ',';
            first = false;
            os << plane_id;
        }
        os << "].";
        throw std::runtime_error(os.str());
    }

    std::vector<size_t> front_cells;
    std::vector<size_t> back_cells;
    for(const size_t cell_id : cell_ids) {
        const int8_t s = arrangement.cells.at(cell_id).plane_signs.at(best_plane);
        if(s >= 0) front_cells.push_back(cell_id);
        if(s <= 0) back_cells.push_back(cell_id);
    }

    remaining_planes.erase(best_plane);
    auto front = build_bsp_from_classified_cells<T, I>(arrangement, std::move(front_cells), remaining_planes, depth + 1);
    auto back = build_bsp_from_classified_cells<T, I>(arrangement, std::move(back_cells), remaining_planes, depth + 1);

    if(front && back && front->type != NT::Partition && back->type != NT::Partition && front->type == back->type) {
        return front;
    }

    return std::make_unique<Node>(bsp_plane_from_exact_plane<T>(arrangement.planes.at(best_plane)),
                                   std::move(front),
                                   std::move(back));
}

void validate_arrangement(const PlanarArrangement &arrangement,
                          size_t arrangement_id) {
    for(size_t he_id = 0; he_id < arrangement.half_edges.size(); ++he_id) {
        const auto &he = arrangement.half_edges[he_id];
        const auto fail = [&](const char *what) {
            std::ostringstream os;
            os << "validate_arrangement: planar arrangement " << arrangement_id
               << " half-edge " << he_id << ' ' << what << '.';
            throw std::runtime_error(os.str());
        };
        if(he.source >= arrangement.vertices.size()) fail("has an invalid source vertex");
        if(he.target >= arrangement.vertices.size()) fail("has an invalid target vertex");
        if(he.twin >= arrangement.half_edges.size()) fail("has an invalid twin");
        if(he.next >= arrangement.half_edges.size()) fail("has an invalid next edge");
        if(he.prev >= arrangement.half_edges.size()) fail("has an invalid previous edge");
        if(he.face >= arrangement.faces.size()) fail("has an invalid face");
        const auto &twin = arrangement.half_edges[he.twin];
        if(twin.twin != he_id) fail("does not have reciprocal twin linkage");
        if(twin.source != he.target || twin.target != he.source) fail("has inconsistent twin endpoints");
        if(arrangement.half_edges[he.next].prev != he_id) fail("does not have reciprocal next/previous linkage");
        if(arrangement.half_edges[he.prev].next != he_id) fail("does not have reciprocal previous/next linkage");
    }

    for(size_t face_id = 0; face_id < arrangement.faces.size(); ++face_id) {
        const auto &face = arrangement.faces[face_id];
        if(face.half_edges.empty()) {
            std::ostringstream os;
            os << "validate_arrangement: planar arrangement " << arrangement_id
               << " face " << face_id << " has no half-edges.";
            throw std::runtime_error(os.str());
        }
        for(const size_t he_id : face.half_edges) {
            if(he_id >= arrangement.half_edges.size() || arrangement.half_edges[he_id].face != face_id) {
                std::ostringstream os;
                os << "validate_arrangement: planar arrangement " << arrangement_id
                   << " face " << face_id << " references an invalid half-edge cycle.";
                throw std::runtime_error(os.str());
            }
        }
    }
}

void validate_arrangement(const GlobalCellArrangement3D &arrangement) {
    if(arrangement.planes.size() != arrangement.plane_keys.size()) {
        throw std::runtime_error("validate_arrangement: global arrangement plane/key count mismatch.");
    }
    for(size_t cell_id = 0; cell_id < arrangement.cells.size(); ++cell_id) {
        const auto &cell = arrangement.cells[cell_id];
        if(cell.plane_signs.size() != arrangement.planes.size()) {
            std::ostringstream os;
            os << "validate_arrangement: global cell " << cell_id
               << " sign vector has " << cell.plane_signs.size()
               << " entries for " << arrangement.planes.size() << " planes.";
            throw std::runtime_error(os.str());
        }
        if(cell.classification == ArrangementCellClass::Unknown) {
            std::ostringstream os;
            os << "validate_arrangement: global cell " << cell_id << " is unclassified.";
            throw std::runtime_error(os.str());
        }
        for(const size_t adjacent : cell.adjacent_cells) {
            if(adjacent >= arrangement.cells.size()) {
                std::ostringstream os;
                os << "validate_arrangement: global cell " << cell_id
                   << " references invalid adjacent cell " << adjacent << '.';
                throw std::runtime_error(os.str());
            }
            if(arrangement.cells[adjacent].adjacent_cells.count(cell_id) == 0) {
                std::ostringstream os;
                os << "validate_arrangement: global cells " << cell_id << " and "
                   << adjacent << " do not have reciprocal adjacency.";
                throw std::runtime_error(os.str());
            }
        }
    }
    for(size_t patch_id = 0; patch_id < arrangement.patches.size(); ++patch_id) {
        const auto &patch = arrangement.patches[patch_id];
        if(patch.plane_id >= arrangement.planes.size()
        || patch.front_cell >= arrangement.cells.size()
        || patch.back_cell >= arrangement.cells.size()) {
            std::ostringstream os;
            os << "validate_arrangement: global patch " << patch_id
               << " references invalid plane or cell ids.";
            throw std::runtime_error(os.str());
        }
    }
}

template <class T, class I>
void validate_bsp_tree_at(const typename bsp_tree_volume<T, I>::Node *node,
                          const std::string &path) {
    using NT = typename bsp_tree_volume<T, I>::NodeType;
    if(!node) {
        throw std::runtime_error("validate_bsp_tree: null node at path " + path + '.');
    }
    if(node->type != NT::Partition) return;
    if(!node->front || !node->back) {
        throw std::runtime_error("validate_bsp_tree: partition node has a null child at path " + path + '.');
    }
    const auto &p = node->partition_plane;
    const auto n = p.normal();
    if(!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z)
    || (n.x == static_cast<T>(0) && n.y == static_cast<T>(0) && n.z == static_cast<T>(0))) {
        throw std::runtime_error("validate_bsp_tree: partition node has a degenerate plane at path " + path + '.');
    }
    if(node->front->type != NT::Partition && node->back->type != NT::Partition && node->front->type == node->back->type) {
        throw std::runtime_error("validate_bsp_tree: partition node has reducible equal leaf children at path " + path + '.');
    }
    validate_bsp_tree_at<T, I>(node->front.get(), path + ".front");
    validate_bsp_tree_at<T, I>(node->back.get(), path + ".back");
}

template <class T, class I>
void validate_bsp_tree(const typename bsp_tree_volume<T, I>::Node *node) {
    validate_bsp_tree_at<T, I>(node, "root");
}


// ---- BSP -> Mesh conversion helpers ----

struct ExactHalfspaceConstraint {
    ExactPlane3 plane;
    // +1 keeps the stored BSP front side, -1 keeps the stored BSP back side.
    // The classified-cell builder partitions exact plane eval >= 0 to front
    // and eval <= 0 to back.
    int side = +1;
};

struct ExactBoundaryFace {
    std::vector<ExactPoint3> vertices;
    ExactPoint3 normal;
};

bool exact_constraint_satisfied(const ExactHalfspaceConstraint &constraint,
                                const ExactPoint3 &p) {
    const int s = constraint.plane.eval(p).sign();
    return constraint.side > 0 ? (s >= 0) : (s <= 0);
}

template <class T, class I>
void collect_leaf_constraints(
    const typename bsp_tree_volume<T, I>::Node *node,
    typename bsp_tree_volume<T, I>::NodeType target,
    std::vector<ExactHalfspaceConstraint> constraints,
    std::vector<std::vector<ExactHalfspaceConstraint>> &regions) {
    using NT = typename bsp_tree_volume<T, I>::NodeType;
    if(!node) {
        if(target == NT::Out) regions.push_back(std::move(constraints));
        return;
    }
    if(node->type != NT::Partition) {
        if(node->type == target) regions.push_back(std::move(constraints));
        return;
    }

    const ExactPlane3 plane = ExactPlane3::from_points(
        ExactPoint3::from_vec3(node->partition_plane.anchors[0]),
        ExactPoint3::from_vec3(node->partition_plane.anchors[1]),
        ExactPoint3::from_vec3(node->partition_plane.anchors[2]));

    auto front_constraints = constraints;
    front_constraints.push_back({plane, +1});
    collect_leaf_constraints<T, I>(node->front.get(), target,
                                   std::move(front_constraints), regions);

    constraints.push_back({plane, -1});
    collect_leaf_constraints<T, I>(node->back.get(), target,
                                   std::move(constraints), regions);
}

std::vector<ExactPoint3> unique_exact_points(std::vector<ExactPoint3> points) {
    std::sort(points.begin(), points.end(), exact_point_less);
    points.erase(std::unique(points.begin(), points.end()), points.end());
    return points;
}

template <class T>
std::vector<ExactPoint3> sort_points_on_plane(std::vector<ExactPoint3> points,
                                              const ExactPoint3 &normal) {
    if(points.size() < 3) return points;

    ExactPoint3 centroid;
    const ExactScalar inv_n = ExactScalar(1) / ExactScalar(static_cast<int64_t>(points.size()));
    for(const auto &p : points) centroid = centroid + p * inv_n;

    const vec3<T> n = exact_point_to_vec3<T>(normal).unit();
    const vec3<T> x_axis(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
    const vec3<T> y_axis(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
    vec3<T> u = (std::abs(n.Dot(x_axis)) < static_cast<T>(0.9))
              ? n.Cross(x_axis).unit()
              : n.Cross(y_axis).unit();
    vec3<T> v = n.Cross(u);
    const vec3<T> c = exact_point_to_vec3<T>(centroid);

    std::sort(points.begin(), points.end(), [&](const ExactPoint3 &lhs, const ExactPoint3 &rhs) {
        const vec3<T> dl = exact_point_to_vec3<T>(lhs) - c;
        const vec3<T> dr = exact_point_to_vec3<T>(rhs) - c;
        const T al = std::atan2(dl.Dot(v), dl.Dot(u));
        const T ar = std::atan2(dr.Dot(v), dr.Dot(u));
        if(al != ar) return al < ar;
        return exact_point_less(lhs, rhs);
    });
    return points;
}

template <class T>
std::vector<ExactPoint3> build_boundary_polygon_from_constraints(
    const ExactPlane3 &support_plane,
    const std::vector<ExactHalfspaceConstraint> &constraints,
    const ExactPoint3 &outward_normal) {
    std::vector<ExactPoint3> candidates;
    for(size_t i = 0; i < constraints.size(); ++i) {
        for(size_t j = i + 1; j < constraints.size(); ++j) {
            try {
                const ExactPoint3 p = intersect_plane_plane_plane_exact(
                    support_plane, constraints[i].plane, constraints[j].plane);
                if(!support_plane.eval(p).is_zero()) continue;
                bool keep = true;
                for(const auto &constraint : constraints) {
                    if(!exact_constraint_satisfied(constraint, p)) {
                        keep = false;
                        break;
                    }
                }
                if(keep) candidates.push_back(p);
            } catch(const std::invalid_argument &) {
            }
        }
    }

    candidates = unique_exact_points(std::move(candidates));
    if(candidates.size() < 3) return {};
    candidates = sort_points_on_plane<T>(std::move(candidates), outward_normal);

    const vec3<T> desired = exact_point_to_vec3<T>(outward_normal);
    const vec3<T> a = exact_point_to_vec3<T>(candidates[0]);
    const vec3<T> b = exact_point_to_vec3<T>(candidates[1]);
    const vec3<T> c = exact_point_to_vec3<T>(candidates[2]);
    if((b - a).Cross(c - a).Dot(desired) < static_cast<T>(0)) {
        std::reverse(candidates.begin(), candidates.end());
    }
    return candidates;
}

template <class T, class I>
void extract_boundary_faces_from_tree(
    const typename bsp_tree_volume<T, I>::Node *node,
    const std::vector<ExactHalfspaceConstraint> &path_constraints,
    std::vector<ExactBoundaryFace> &faces) {
    using NT = typename bsp_tree_volume<T, I>::NodeType;
    if(!node || node->type != NT::Partition) return;

    const ExactPlane3 support_plane = ExactPlane3::from_points(
        ExactPoint3::from_vec3(node->partition_plane.anchors[0]),
        ExactPoint3::from_vec3(node->partition_plane.anchors[1]),
        ExactPoint3::from_vec3(node->partition_plane.anchors[2]));
    const ExactPoint3 normal = plane_normal_exact(support_plane);

    std::vector<std::vector<ExactHalfspaceConstraint>> front_in, front_out, back_in, back_out;
    collect_leaf_constraints<T, I>(node->front.get(), NT::In, {}, front_in);
    collect_leaf_constraints<T, I>(node->front.get(), NT::Out, {}, front_out);
    collect_leaf_constraints<T, I>(node->back.get(), NT::In, {}, back_in);
    collect_leaf_constraints<T, I>(node->back.get(), NT::Out, {}, back_out);

    const auto emit_pairs = [&](const std::vector<std::vector<ExactHalfspaceConstraint>> &inside_regions,
                                const std::vector<std::vector<ExactHalfspaceConstraint>> &outside_regions,
                                const ExactPoint3 &outward) {
        for(const auto &inside : inside_regions) {
            for(const auto &outside : outside_regions) {
                std::vector<ExactHalfspaceConstraint> constraints = path_constraints;
                constraints.insert(constraints.end(), inside.begin(), inside.end());
                constraints.insert(constraints.end(), outside.begin(), outside.end());
                auto polygon = build_boundary_polygon_from_constraints<T>(support_plane, constraints, outward);
                if(polygon.size() >= 3) faces.push_back({std::move(polygon), outward});
            }
        }
    };

    emit_pairs(front_in, back_out, normal);
    emit_pairs(back_in, front_out, normal * ExactScalar(-1));

    auto front_path = path_constraints;
    front_path.push_back({support_plane, +1});
    extract_boundary_faces_from_tree<T, I>(node->front.get(), front_path, faces);

    auto back_path = path_constraints;
    back_path.push_back({support_plane, -1});
    extract_boundary_faces_from_tree<T, I>(node->back.get(), back_path, faces);
}

template <class T>
void triangulate_exact_fan(const std::vector<ExactPoint3> &poly,
                           std::vector<std::array<ExactPoint3, 3>> &out) {
    if(poly.size() < 3) return;
    for(size_t i = 1; i + 1 < poly.size(); ++i) {
        const ExactPoint3 normal = cross_exact(poly[i] - poly[0], poly[i + 1] - poly[0]);
        if(normal.x.is_zero() && normal.y.is_zero() && normal.z.is_zero()) continue;
        out.push_back({{poly[0], poly[i], poly[i + 1]}});
    }
}

template <class T, class I>
bool tree_has_extractable_bounded_boundary(const typename bsp_tree_volume<T, I>::Node *node) {
    using NT = typename bsp_tree_volume<T, I>::NodeType;
    if(!node || node->type == NT::Out) return false;
    if(node->type == NT::In) return true;

    std::vector<ExactBoundaryFace> faces;
    extract_boundary_faces_from_tree<T, I>(node, {}, faces);
    for(const auto &face : faces) {
        std::vector<std::array<ExactPoint3, 3>> tris;
        triangulate_exact_fan<T>(face.vertices, tris);
        if(!tris.empty()) return true;
    }
    return false;
}

template <class T, class I>
NodePtr<T, I> canonicalize_empty_bounded_result(NodePtr<T, I> node) {
    using NT = typename bsp_tree_volume<T, I>::NodeType;
    if(!node) return make_out_node<T, I>();
    if(node->type != NT::Partition) return node;
    if(tree_has_extractable_bounded_boundary<T, I>(node.get())) return node;
    return make_out_node<T, I>();
}

template <class T, class I>
void validate_output_mesh(const fv_surface_mesh<T, I> &mesh,
                          const std::vector<std::array<ExactPoint3, 3>> &exact_tris) {
    if(mesh.faces.empty() && mesh.vertices.empty()) return;
    const auto exact_edge_less = [](const std::pair<ExactPoint3, ExactPoint3> &lhs,
                                    const std::pair<ExactPoint3, ExactPoint3> &rhs) {
        if(exact_point_less(lhs.first, rhs.first)) return true;
        if(exact_point_less(rhs.first, lhs.first)) return false;
        return exact_point_less(lhs.second, rhs.second);
    };
    std::map<std::pair<ExactPoint3, ExactPoint3>, size_t, decltype(exact_edge_less)> exact_edges(exact_edge_less);
    for(const auto &tri : exact_tris) {
        for(size_t ei = 0; ei < 3; ++ei) {
            ExactPoint3 a = tri[ei];
            ExactPoint3 b = tri[(ei + 1) % 3];
            if(exact_point_less(b, a)) std::swap(a, b);
            ++exact_edges[{a, b}];
        }
    }
    bool exact_closed = true;
    for(const auto &entry : exact_edges) {
        if(entry.second != 2) {
            exact_closed = false;
            break;
        }
    }

    if(!HasOnlyFiniteVertices(mesh)) {
        throw std::runtime_error("validate_output_mesh: output mesh contains non-finite vertices.");
    }
    if(!IsTriangularMesh(mesh)) {
        throw std::runtime_error("validate_output_mesh: output mesh contains non-triangular faces.");
    }
    if(!HasValidFaceIndices(mesh)) {
        throw std::runtime_error("validate_output_mesh: output mesh contains invalid face indices.");
    }
    if(!HasNoDegenerateFaces(mesh)) {
        throw std::runtime_error("validate_output_mesh: output mesh contains degenerate faces.");
    }

    if(!exact_closed) return;

    if(!IsClosedManifold(mesh)) {
        throw std::runtime_error("validate_output_mesh: output mesh is not a closed manifold.");
    }
    if(!HasConsistentOrientation(mesh)) {
        throw std::runtime_error("validate_output_mesh: output mesh has inconsistent orientation.");
    }
}

} // anonymous namespace


namespace ygor_bsp_tree_exact_test {

bool exact_kernel_self_test() {
    const ExactPoint3 origin{ExactScalar(0), ExactScalar(0), ExactScalar(0)};
    const ExactPoint3 ex{ExactScalar(1), ExactScalar(0), ExactScalar(0)};
    const ExactPoint3 ey{ExactScalar(0), ExactScalar(1), ExactScalar(0)};
    const ExactPlane3 z0 = ExactPlane3::from_points(origin, ex, ey);

    const ExactPoint3 below{ExactScalar(0), ExactScalar(0), ExactScalar(-1)};
    const ExactPoint3 above{ExactScalar(0), ExactScalar(0), ExactScalar(1)};
    const ExactPoint3 seg_hit = intersect_segment_plane_exact(below, above, z0);
    if(seg_hit.x != ExactScalar(0) || seg_hit.y != ExactScalar(0) || seg_hit.z != ExactScalar(0)) return false;

    const ExactPlane3 x2{ExactScalar(1), ExactScalar(0), ExactScalar(0), ExactScalar(-2)};
    const ExactPlane3 y3{ExactScalar(0), ExactScalar(1), ExactScalar(0), ExactScalar(-3)};
    const ExactPlane3 z5{ExactScalar(0), ExactScalar(0), ExactScalar(1), ExactScalar(-5)};
    const ExactPoint3 triple_hit = intersect_plane_plane_plane_exact(x2, y3, z5);
    if(triple_hit.x != ExactScalar(2) || triple_hit.y != ExactScalar(3) || triple_hit.z != ExactScalar(5)) return false;

    const ExactLine3 xy_line = intersect_plane_plane_exact(x2, y3);
    const ExactPoint3 line_hit = intersect_line_plane_exact(xy_line, z5);
    if(line_hit.x != triple_hit.x || line_hit.y != triple_hit.y || line_hit.z != triple_hit.z) return false;

    const ExactScalar one_tenth_binary = ExactScalar::from_binary(0.1);
    if(one_tenth_binary == (ExactScalar(1) / ExactScalar(10))) return false;
    if(ExactScalar::from_binary(0.5) != (ExactScalar(1) / ExactScalar(2))) return false;

    const ExactVertex3 original_a{origin, {ExactVertexKind::OriginalVertex, {{7, 0, 0}}}};
    const ExactVertex3 original_b{ex, {ExactVertexKind::OriginalVertex, {{7, 0, 0}}}};
    if(exact_vertex_less(original_a, original_b) || exact_vertex_less(original_b, original_a)) return false;

    const int sign = filtered_orient3d_sign(vec3<double>(0.0, 0.0, 0.0),
                                            vec3<double>(1.0, 0.0, 0.0),
                                            vec3<double>(0.0, 1.0, 0.0),
                                            vec3<double>(0.0, 0.0, 1.0));
    if(sign >= 0) return false;

    const ExactScalar h = ExactScalar(1) / ExactScalar(2);
    const std::vector<PlanarSegment> segments = {
        {{ExactScalar(0), ExactScalar(0)}, {ExactScalar(1), ExactScalar(0)}, 0},
        {{ExactScalar(1), ExactScalar(0)}, {ExactScalar(1), ExactScalar(1)}, 0},
        {{ExactScalar(1), ExactScalar(1)}, {ExactScalar(0), ExactScalar(1)}, 0},
        {{ExactScalar(0), ExactScalar(1)}, {ExactScalar(0), ExactScalar(0)}, 0},
        {{h, ExactScalar(0)}, {h, ExactScalar(1)}, 1}
    };
    const PlanarArrangement arrangement = build_planar_arrangement(segments);
    if(arrangement.vertices.size() != 6) return false;
    size_t bounded_faces = 0;
    for(const auto &face : arrangement.faces) {
        if(face.bounded) ++bounded_faces;
    }
    if(bounded_faces != 2) return false;
    for(size_t he_id = 0; he_id < arrangement.half_edges.size(); ++he_id) {
        const auto &he = arrangement.half_edges[he_id];
        if(he.twin >= arrangement.half_edges.size()) return false;
        if(arrangement.half_edges[he.twin].twin != he_id) return false;
        if(he.next >= arrangement.half_edges.size()) return false;
        if(he.prev >= arrangement.half_edges.size()) return false;
    }

    const std::array<vec3<double>, 4> tv = {{
        vec3<double>(0.0, 0.0, 0.0),
        vec3<double>(1.0, 0.0, 0.0),
        vec3<double>(0.0, 1.0, 0.0),
        vec3<double>(0.0, 0.0, 1.0)
    }};
    const std::array<std::array<size_t, 3>, 4> tf = {{
        {{0, 2, 1}},
        {{0, 1, 3}},
        {{0, 3, 2}},
        {{1, 2, 3}}
    }};
    std::vector<TriangleRec<double>> tetra_tris;
    for(size_t fi = 0; fi < tf.size(); ++fi) {
        TriangleRec<double> tri;
        tri.v = {{tv[tf[fi][0]], tv[tf[fi][1]], tv[tf[fi][2]]}};
        tri.pl = plane_from_triangle(tri.v[0], tri.v[1], tri.v[2]);
        tri.exact_plane = ExactPlane3::from_points(ExactPoint3::from_vec3(tri.v[0]),
                                                   ExactPoint3::from_vec3(tri.v[1]),
                                                   ExactPoint3::from_vec3(tri.v[2]));
        tri.original_face_id = fi;
        tetra_tris.push_back(tri);
    }
    const auto tetra_planar = build_planar_arrangements_for_support_planes(tetra_tris);
    const auto tetra_arrangement = build_global_cell_arrangement(tetra_tris, tetra_planar);
    if(tetra_arrangement.planes.size() != 4) return false;
    if(tetra_arrangement.patches.size() != 4) return false;
    if(tetra_arrangement.cells.empty()) return false;
    for(const auto &patch : tetra_arrangement.patches) {
        if(patch.plane_id >= tetra_arrangement.planes.size()) return false;
        if(patch.front_cell >= tetra_arrangement.cells.size()) return false;
        if(patch.back_cell >= tetra_arrangement.cells.size()) return false;
        if(tetra_arrangement.cells[patch.front_cell].classification != ArrangementCellClass::Out) return false;
        if(tetra_arrangement.cells[patch.back_cell].classification != ArrangementCellClass::In) return false;
        if(tetra_arrangement.cells[patch.front_cell].adjacent_cells.count(patch.back_cell) != 1) return false;
        if(tetra_arrangement.cells[patch.back_cell].adjacent_cells.count(patch.front_cell) != 1) return false;
    }

    using TestBsp = bsp_tree_volume<double, uint64_t>;
    using TestNode = TestBsp::Node;
    using TestNT = TestBsp::NodeType;
    const bsp_plane<double> z_up(vec3<double>(0.0, 0.0, 0.0),
                                 vec3<double>(1.0, 0.0, 0.0),
                                 vec3<double>(0.0, 1.0, 0.0));
    const bsp_plane<double> z_down(vec3<double>(0.0, 0.0, 0.0),
                                   vec3<double>(0.0, 1.0, 0.0),
                                   vec3<double>(1.0, 0.0, 0.0));
    const auto eval_tree = [](const TestNode *n, const vec3<double> &p, const auto &recurse) -> TestNT {
        if(!n) return TestNT::Out;
        if(n->type != TestNT::Partition) return n->type;
        return (classify_point(n->partition_plane, p) >= 0)
            ? recurse(n->front.get(), p, recurse)
            : recurse(n->back.get(), p, recurse);
    };
    const auto make_lower = [&]() {
        return std::make_unique<TestNode>(z_up,
                                          std::make_unique<TestNode>(TestNT::In),
                                          std::make_unique<TestNode>(TestNT::Out));
    };
    const auto make_lower_reversed = [&]() {
        return std::make_unique<TestNode>(z_down,
                                          std::make_unique<TestNode>(TestNT::Out),
                                          std::make_unique<TestNode>(TestNT::In));
    };
    const vec3<double> above_pt(0.0, 0.0, 1.0);
    const vec3<double> below_pt(0.0, 0.0, -1.0);
    auto aligned_intersection = merge_bsp<double, uint64_t>(make_lower(), make_lower(), 1);
    if(eval_tree(aligned_intersection.get(), above_pt, eval_tree) != TestNT::Out) return false;
    if(eval_tree(aligned_intersection.get(), below_pt, eval_tree) != TestNT::In) return false;

    auto anti_aligned_intersection = merge_bsp<double, uint64_t>(make_lower(), make_lower_reversed(), 1);
    if(eval_tree(anti_aligned_intersection.get(), above_pt, eval_tree) != TestNT::Out) return false;
    if(eval_tree(anti_aligned_intersection.get(), below_pt, eval_tree) != TestNT::In) return false;

    auto anti_aligned_subtraction = merge_bsp<double, uint64_t>(make_lower(), make_lower_reversed(), 2);
    if(eval_tree(anti_aligned_subtraction.get(), above_pt, eval_tree) != TestNT::Out) return false;
    if(eval_tree(anti_aligned_subtraction.get(), below_pt, eval_tree) != TestNT::Out) return false;

    return true;
}

} // namespace ygor_bsp_tree_exact_test


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
    : root(r ? std::move(r) : make_out_node<T, I>()) {}

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
    return (!root || root->type == NodeType::Out);
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
    r = collapse_deep_uniform<T, I>(std::move(r));
    r = canonicalize_empty_bounded_result<T, I>(std::move(r));
    validate_bsp_tree<T, I>(r.get());
    return bsp_tree_volume(std::move(r));
}

template <class T, class I>
bsp_tree_volume<T, I>
bsp_tree_volume<T, I>::boolean_intersection(const bsp_tree_volume &other) const {
    if(!root || !other.root)
        return bsp_tree_volume();
    auto r = merge_bsp<T, I>(clone_node<T, I>(root.get()),
                             clone_node<T, I>(other.root.get()), 1);
    r = collapse_deep_uniform<T, I>(std::move(r));
    r = canonicalize_empty_bounded_result<T, I>(std::move(r));
    validate_bsp_tree<T, I>(r.get());
    return bsp_tree_volume(std::move(r));
}

template <class T, class I>
bsp_tree_volume<T, I>
bsp_tree_volume<T, I>::boolean_subtraction(const bsp_tree_volume &other) const {
    if(!root) return bsp_tree_volume();
    if(!other.root) return *this;
    auto r = merge_bsp<T, I>(clone_node<T, I>(root.get()),
                             clone_node<T, I>(other.root.get()), 2);
    r = collapse_deep_uniform<T, I>(std::move(r));
    r = canonicalize_empty_bounded_result<T, I>(std::move(r));
    validate_bsp_tree<T, I>(r.get());
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
    r = collapse_deep_uniform<T, I>(std::move(r));
    r = canonicalize_empty_bounded_result<T, I>(std::move(r));
    validate_bsp_tree<T, I>(r.get());
    return bsp_tree_volume(std::move(r));
}


// ---- Conversion: fv_surface_mesh -> bsp_tree_volume ----

template <class T, class I>
bsp_tree_volume<T, I>
bsp_tree_volume<T, I>::from_fv_surface_mesh(
    const fv_surface_mesh<T, I> &mesh,
    std::optional<uint64_t> seed) {
    (void)seed;

    if(mesh.faces.empty() || mesh.vertices.empty())
        return bsp_tree_volume(make_out_node<T, I>());

    fv_surface_mesh<T, I> working_mesh = mesh;
    working_mesh.convert_to_triangles();

    if(!HasOnlyFiniteVertices(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh contains non-finite vertices.");

    if(working_mesh.faces.empty())
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh contains no triangulatable faces.");

    if(!IsTriangularMesh(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh must contain only triangular faces.");
    if(!HasValidFaceIndices(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh contains out-of-range face indices.");
    working_mesh.remove_degenerate_faces();

    if(working_mesh.faces.empty())
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh contains only degenerate faces.");

    if(!HasNoDegenerateFaces(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: mesh contains degenerate faces.");

    std::vector<CanonicalTriangleRec<T, I>> canonical_triangles;
    canonicalize_mesh_for_bsp<T, I>(working_mesh, canonical_triangles);

    if(!IsClosedManifold(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: canonicalized mesh is not a closed manifold.");
    if(!HasConsistentOrientation(working_mesh))
        throw std::invalid_argument("bsp_tree_volume::from_fv_surface_mesh: canonicalized mesh faces do not have a consistent orientation.");

    std::vector<TriangleRec<T>> tri_recs;

    tri_recs.reserve(working_mesh.faces.size());

    for(size_t fi = 0; fi < working_mesh.faces.size(); ++fi) {
        const auto &f = working_mesh.faces[fi];
        if(f.size() != 3) continue;

        const I i0 = f[0], i1 = f[1], i2 = f[2];

        const vec3<T> &v0 = working_mesh.vertices[i0];
        const vec3<T> &v1 = working_mesh.vertices[i1];
        const vec3<T> &v2 = working_mesh.vertices[i2];

        TriangleRec<T> rec;
        rec.v = {{v0, v1, v2}};
        rec.pl = plane_from_triangle(v0, v1, v2);
        try {
            if(fi < canonical_triangles.size()) {
                rec.exact_plane = ExactPlane3::from_points(canonical_triangles[fi].exact[0],
                                                           canonical_triangles[fi].exact[1],
                                                           canonical_triangles[fi].exact[2]);
                rec.original_face_id = canonical_triangles[fi].original_face_id;
            } else {
                rec.exact_plane = ExactPlane3::from_points(ExactPoint3::from_vec3(v0),
                                                           ExactPoint3::from_vec3(v1),
                                                           ExactPoint3::from_vec3(v2));
                rec.original_face_id = fi;
            }
        } catch(const std::invalid_argument &) {
            throw std::invalid_argument(face_id_message<I>("zero-area face under exact plane construction", fi));
        }
        tri_recs.push_back(rec);
    }

    if(tri_recs.empty())
        throw std::runtime_error("bsp_tree_volume::from_fv_surface_mesh: BSP construction produced no valid triangle records.");

    const auto planar_arrangements = build_planar_arrangements_for_support_planes(tri_recs);
    if(planar_arrangements.empty()) {
        throw std::runtime_error("bsp_tree_volume::from_fv_surface_mesh: planar support arrangement construction produced no planes.");
    }
    for(size_t arrangement_id = 0; arrangement_id < planar_arrangements.size(); ++arrangement_id) {
        validate_arrangement(planar_arrangements[arrangement_id], arrangement_id);
    }
    const auto global_arrangement = build_global_cell_arrangement(tri_recs, planar_arrangements);
    validate_arrangement(global_arrangement);
    const auto bsp_arrangement = canonicalize_cells_for_bsp(global_arrangement);
    validate_arrangement(bsp_arrangement);
    if(bsp_arrangement.cells.empty()) {
        throw std::runtime_error("bsp_tree_volume::from_fv_surface_mesh: global 3D cell arrangement construction produced no cells.");
    }

    std::vector<size_t> cell_ids;
    cell_ids.reserve(bsp_arrangement.cells.size());
    for(size_t i = 0; i < bsp_arrangement.cells.size(); ++i) {
        cell_ids.push_back(i);
    }
    std::set<size_t> remaining_planes;
    for(size_t i = 0; i < bsp_arrangement.planes.size(); ++i) {
        remaining_planes.insert(i);
    }

    auto root = build_bsp_from_classified_cells<T, I>(
        bsp_arrangement, std::move(cell_ids), std::move(remaining_planes), 0);
    validate_bsp_tree<T, I>(root.get());

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
        throw std::runtime_error("bsp_tree_volume::to_fv_surface_mesh: cannot emit a bounded mesh for an unbounded In volume.");
    }

    std::vector<ExactBoundaryFace> faces;
    extract_boundary_faces_from_tree<T, I>(root.get(), {}, faces);

    std::vector<std::array<ExactPoint3, 3>> tris;
    for(const auto &face : faces) {
        triangulate_exact_fan<T>(face.vertices, tris);
    }
    if(tris.empty()) return mesh;

    std::map<ExactPoint3, I, decltype(&exact_point_less)> vert_map(&exact_point_less);
    const auto get_or_add_vertex = [&](const ExactPoint3 &p) -> I {
        const auto it = vert_map.find(p);
        if(it != vert_map.end()) return it->second;
        const I idx = static_cast<I>(mesh.vertices.size());
        mesh.vertices.push_back(exact_point_to_vec3<T>(p));
        vert_map.emplace(p, idx);
        return idx;
    };

    for(const auto &tri : tris) {
        std::vector<I> face;
        face.push_back(get_or_add_vertex(tri[0]));
        face.push_back(get_or_add_vertex(tri[1]));
        face.push_back(get_or_add_vertex(tri[2]));
        if(face[0] != face[1] && face[1] != face[2] && face[2] != face[0]) {
            mesh.faces.push_back(std::move(face));
        }
    }

    OrientFaces(mesh);
    mesh.recreate_involved_face_index();
    validate_output_mesh<T, I>(mesh, tris);
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
