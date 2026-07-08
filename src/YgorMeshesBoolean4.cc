//YgorMeshesBoolean4.cc - Written by hal clark in 2026.
//
// Surface mesh Boolean engine backed by an explicit split-facet arrangement.

#include <cstdint>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <tuple>
#include <stdexcept>
#include <string>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorMeshesBoolean4.h"
#include "YgorMeshesVerification.h"


namespace {

enum class Boolean4Operand : uint8_t {
    Lhs,
    Rhs
};


const char *
Boolean4OperandName(Boolean4Operand operand) {
    return operand == Boolean4Operand::Lhs ? "lhs" : "rhs";
}


struct Boolean4SourceVertexRef {
    Boolean4Operand operand = Boolean4Operand::Lhs;
    uint64_t vertex_id = 0;

    bool operator<(const Boolean4SourceVertexRef &rhs) const {
        return std::tie(operand, vertex_id) < std::tie(rhs.operand, rhs.vertex_id);
    }
};


struct Boolean4SourceFaceRef {
    Boolean4Operand operand = Boolean4Operand::Lhs;
    uint64_t face_id = 0;

    bool operator<(const Boolean4SourceFaceRef &rhs) const {
        return std::tie(operand, face_id) < std::tie(rhs.operand, rhs.face_id);
    }
};


// Deterministic snap-rounding key used only for constructed-topology identity.
// Boolean4 promotes both float and double input coordinates to long double while
// forming this shared 1e-12 grid key, then stores the rounded integer coordinate
// as the only topology key. Floating coordinates may be retained for final
// emission, but they are not topology keys. Inputs whose scaled coordinates exceed
// int64_t are clamped to the grid boundary deterministically; the input validator
// separately rejects non-finite coordinates before any topology construction.
struct Boolean4SnapCoord {
    int64_t x = 0;
    int64_t y = 0;
    int64_t z = 0;
    int32_t binary_exponent = 0;

    bool operator<(const Boolean4SnapCoord &rhs) const {
        return std::tie(binary_exponent, x, y, z)
             < std::tie(rhs.binary_exponent, rhs.x, rhs.y, rhs.z);
    }
};


enum class Boolean4VertexKind : uint8_t {
    InputVertex,
    TriangleTriangleIntersection,
    CoplanarConstraintVertex
};


struct Boolean4VertexRecord {
    uint64_t stable_id = 0;
    Boolean4VertexKind kind = Boolean4VertexKind::InputVertex;
    Boolean4SnapCoord key;
    std::vector<Boolean4SourceVertexRef> source_vertices;
    std::vector<Boolean4SourceFaceRef> source_faces;

    bool operator<(const Boolean4VertexRecord &rhs) const {
        return std::tie(key, kind, stable_id)
             < std::tie(rhs.key, rhs.kind, rhs.stable_id);
    }
};


struct Boolean4DirectedHalfedge {
    uint64_t stable_id = 0;
    uint64_t origin_vertex_id = 0;
    uint64_t target_vertex_id = 0;
    uint64_t face_fragment_id = 0;
    uint64_t twin_halfedge_id = 0;
    uint64_t next_halfedge_id = 0;

    bool operator<(const Boolean4DirectedHalfedge &rhs) const {
        return std::tie(origin_vertex_id, target_vertex_id, face_fragment_id, stable_id)
             < std::tie(rhs.origin_vertex_id, rhs.target_vertex_id, rhs.face_fragment_id, rhs.stable_id);
    }
};


struct Boolean4ComponentId {
    uint64_t stable_id = 0;

    bool operator<(const Boolean4ComponentId &rhs) const {
        return stable_id < rhs.stable_id;
    }
};


struct Boolean4ShellId {
    uint64_t stable_id = 0;

    bool operator<(const Boolean4ShellId &rhs) const {
        return stable_id < rhs.stable_id;
    }
};


struct Boolean4FaceFragment {
    uint64_t stable_id = 0;
    Boolean4SourceFaceRef source_face;
    Boolean4ComponentId component;
    Boolean4ShellId shell;
    bool preserves_source_orientation = true;
    std::vector<uint64_t> boundary_halfedge_ids;
    std::vector<uint64_t> triangulated_face_ids;

    bool operator<(const Boolean4FaceFragment &rhs) const {
        return std::tie(source_face, stable_id)
             < std::tie(rhs.source_face, rhs.stable_id);
    }
};


template <class T>
Boolean4SnapCoord
Boolean4SnapKey(const vec3<T> &p);


template <class T>
struct Boolean4ConstraintSegment {
    vec3<T> a;
    vec3<T> b;

    bool operator<(const Boolean4ConstraintSegment &rhs) const {
        const auto lhs_key_a = Boolean4SnapKey(a);
        const auto lhs_key_b = Boolean4SnapKey(b);
        const auto rhs_key_a = Boolean4SnapKey(rhs.a);
        const auto rhs_key_b = Boolean4SnapKey(rhs.b);
        return std::tie(lhs_key_a, lhs_key_b) < std::tie(rhs_key_a, rhs_key_b);
    }
};


template <class T>
struct Boolean4ArrangementFacet {
    uint64_t stable_id = 0;
    Boolean4SourceFaceRef source_face;
    std::array<uint64_t, 3> vertex_ids = { { 0, 0, 0 } };

    bool operator<(const Boolean4ArrangementFacet &rhs) const {
        return std::tie(source_face, vertex_ids, stable_id)
             < std::tie(rhs.source_face, rhs.vertex_ids, rhs.stable_id);
    }
};


template <class T>
struct Boolean4SplitTriangleResult {
    std::vector<Boolean4VertexRecord> vertices;
    std::vector<vec3<T>> approximate_vertices;
    std::vector<Boolean4ArrangementFacet<T>> facets;
};


template <class T>
struct Boolean4OperandArrangement {
    std::vector<Boolean4SplitTriangleResult<T>> split_faces;
};


template <class T>
struct Boolean4PairArrangement {
    Boolean4OperandArrangement<T> lhs;
    Boolean4OperandArrangement<T> rhs;
};


enum class Boolean4FacetLocation : uint8_t {
    Outside,
    Inside,
    OnBoundary
};


enum class Boolean4BoundaryFacing : uint8_t {
    None,
    SameFacing,
    OppositeFacing
};


enum class Boolean4FacetDecisionReason : uint8_t {
    LhsOutsideKept,
    LhsInsideKept,
    LhsInsideDiscarded,
    LhsSameBoundaryKeptAsRepresentative,
    LhsSameBoundaryDiscarded,
    LhsOppositeBoundaryKeptForSubtractionContact,
    LhsOppositeBoundaryDiscarded,
    LhsInsideKeptReversedForExclusion,
    RhsOutsideKept,
    RhsInsideKept,
    RhsInsideDiscarded,
    RhsInsideKeptReversed,
    RhsSameBoundaryDiscardedAsDuplicate,
    RhsOppositeBoundaryDiscarded,
    RhsInsideKeptReversedForExclusion
};


template <class T>
struct Boolean4ClassifiedFacet {
    Boolean4FacetLocation location = Boolean4FacetLocation::Outside;
    Boolean4BoundaryFacing boundary_facing = Boolean4BoundaryFacing::None;
    uint64_t split_face_id = 0;
    uint64_t facet_id = 0;
    Boolean4SourceFaceRef source_face;
};


template <class T>
struct Boolean4FacetDecision {
    bool emit = false;
    bool reverse_orientation = false;
    Boolean4Operand operand = Boolean4Operand::Lhs;
    uint64_t split_face_id = 0;
    uint64_t facet_id = 0;
    Boolean4SourceFaceRef source_face;
    Boolean4FacetDecisionReason reason = Boolean4FacetDecisionReason::LhsInsideDiscarded;

    bool operator<(const Boolean4FacetDecision &rhs) const {
        return std::tie(operand, source_face, split_face_id, facet_id, emit, reverse_orientation, reason)
             < std::tie(rhs.operand, rhs.source_face, rhs.split_face_id, rhs.facet_id, rhs.emit, rhs.reverse_orientation, rhs.reason);
    }
};


template <class T>
struct Boolean4OperandFacetDecisions {
    std::vector<Boolean4FacetDecision<T>> facets;
};


template <class T>
struct Boolean4PairFacetDecisions {
    Boolean4OperandFacetDecisions<T> lhs;
    Boolean4OperandFacetDecisions<T> rhs;
};


template <class T>
struct Boolean4OutputFacetCandidate {
    std::array<Boolean4SnapCoord, 3> vertex_keys;
    std::array<vec3<T>, 3> approximate_vertices;
    Boolean4Operand operand = Boolean4Operand::Lhs;
    uint64_t split_face_id = 0;
    uint64_t facet_id = 0;
    Boolean4SourceFaceRef source_face;

    bool operator<(const Boolean4OutputFacetCandidate &rhs) const {
        return std::tie(vertex_keys, operand, source_face, split_face_id, facet_id)
             < std::tie(rhs.vertex_keys, rhs.operand, rhs.source_face, rhs.split_face_id, rhs.facet_id);
    }
};


template <class T>
struct Boolean4OutputVertexCandidate {
    Boolean4SnapCoord key;
    vec3<T> approximate;

    bool operator<(const Boolean4OutputVertexCandidate &rhs) const {
        return std::tie(key, approximate.x, approximate.y, approximate.z)
             < std::tie(rhs.key, rhs.approximate.x, rhs.approximate.y, rhs.approximate.z);
    }
};


struct Boolean4OutputHalfedgeCandidate {
    uint64_t stable_id = 0;
    uint64_t face_id = 0;
    uint64_t origin = 0;
    uint64_t target = 0;
    uint64_t undirected_min = 0;
    uint64_t undirected_max = 0;
    uint64_t next_halfedge_id = 0;

    bool operator<(const Boolean4OutputHalfedgeCandidate &rhs) const {
        return std::tie(undirected_min, undirected_max, origin, target, face_id, stable_id)
             < std::tie(rhs.undirected_min, rhs.undirected_max, rhs.origin, rhs.target, rhs.face_id, rhs.stable_id);
    }
};


template <class T>
struct Boolean4OperandFacetClassification {
    std::vector<Boolean4ClassifiedFacet<T>> facets;
};


template <class T>
struct Boolean4PairFacetClassification {
    Boolean4OperandFacetClassification<T> lhs;
    Boolean4OperandFacetClassification<T> rhs;
};


template <class T>
struct Boolean4AABB {
    vec3<T> min;
    vec3<T> max;
};


template <class T>
struct Boolean4Cuboid {
    T xmin = static_cast<T>(0);
    T ymin = static_cast<T>(0);
    T zmin = static_cast<T>(0);
    T xmax = static_cast<T>(0);
    T ymax = static_cast<T>(0);
    T zmax = static_cast<T>(0);
};


template <class I>
I
Boolean4CheckedIndex(uint64_t value,
                     const char *what);


template <class T, class I>
void
VerifyBoolean4OutputPostconditions(const fv_surface_mesh<T, I> &mesh,
                                   MeshBooleanOperation4 op);


template <class T>
struct Boolean4TriangleAABB {
    uint64_t normalized_face_id = 0;
    Boolean4SourceFaceRef source_face;
    Boolean4AABB<T> bounds;

    bool operator<(const Boolean4TriangleAABB &rhs) const {
        return std::tie(bounds.min.x, bounds.min.y, bounds.min.z,
                        bounds.max.x, bounds.max.y, bounds.max.z,
                        normalized_face_id, source_face)
             < std::tie(rhs.bounds.min.x, rhs.bounds.min.y, rhs.bounds.min.z,
                        rhs.bounds.max.x, rhs.bounds.max.y, rhs.bounds.max.z,
                        rhs.normalized_face_id, rhs.source_face);
    }
};


struct Boolean4TrianglePairCandidate {
    uint64_t lhs_face_id = 0;
    uint64_t rhs_face_id = 0;
    Boolean4SourceFaceRef lhs_source_face;
    Boolean4SourceFaceRef rhs_source_face;

    bool operator<(const Boolean4TrianglePairCandidate &rhs) const {
        return std::tie(lhs_face_id, rhs_face_id, lhs_source_face, rhs_source_face)
             < std::tie(rhs.lhs_face_id, rhs.rhs_face_id, rhs.lhs_source_face, rhs.rhs_source_face);
    }
};


enum class Boolean4TriangleTriangleCase : uint8_t {
    Disjoint,
    ProperSegmentIntersection,
    PointContact,
    SharedEdgeOrPartialOverlappingEdge,
    CoplanarDisjoint,
    CoplanarOverlapArea,
    IdenticalTriangles,
    ReversedIdenticalTriangles
};


const char *
Boolean4TriangleTriangleCaseName(Boolean4TriangleTriangleCase kind) {
    switch(kind) {
        case Boolean4TriangleTriangleCase::Disjoint:
            return "disjoint";
        case Boolean4TriangleTriangleCase::ProperSegmentIntersection:
            return "proper segment intersection";
        case Boolean4TriangleTriangleCase::PointContact:
            return "point contact";
        case Boolean4TriangleTriangleCase::SharedEdgeOrPartialOverlappingEdge:
            return "shared edge or partial overlapping edge";
        case Boolean4TriangleTriangleCase::CoplanarDisjoint:
            return "coplanar disjoint";
        case Boolean4TriangleTriangleCase::CoplanarOverlapArea:
            return "coplanar overlap area";
        case Boolean4TriangleTriangleCase::IdenticalTriangles:
            return "identical triangles";
        case Boolean4TriangleTriangleCase::ReversedIdenticalTriangles:
            return "reversed identical triangles";
    }
    return "invalid triangle-triangle case";
}


struct Boolean4ValidationDiagnostic {
    const char *stage = "validation";
    Boolean4Operand operand = Boolean4Operand::Lhs;
    uint64_t face_id = 0;
    uint64_t vertex_id = 0;
    std::string message;
};


struct Boolean4TrianglePairDiagnostic {
    uint64_t lhs_face_id = 0;
    uint64_t rhs_face_id = 0;
    Boolean4TriangleTriangleCase predicate_case = Boolean4TriangleTriangleCase::Disjoint;
    size_t constructed_vertex_count = 0;
};


struct Boolean4ArrangementSplitDiagnostic {
    Boolean4SourceFaceRef source_face;
    size_t constraint_count = 0;
    size_t vertex_count = 0;
    size_t facet_count = 0;
};


struct Boolean4FacetClassificationDiagnostic {
    Boolean4Operand operand = Boolean4Operand::Lhs;
    uint64_t split_face_id = 0;
    uint64_t facet_id = 0;
    Boolean4SourceFaceRef source_face;
    Boolean4FacetLocation location = Boolean4FacetLocation::Outside;
    Boolean4BoundaryFacing boundary_facing = Boolean4BoundaryFacing::None;
};


struct Boolean4OutputAssemblyDiagnostic {
    uint64_t output_face_id = 0;
    uint64_t output_vertex_id = 0;
    uint64_t source_face_id = 0;
    Boolean4Operand operand = Boolean4Operand::Lhs;
    std::string message;
};


const char *
Boolean4FacetLocationName(Boolean4FacetLocation location) {
    switch(location) {
        case Boolean4FacetLocation::Outside:
            return "outside";
        case Boolean4FacetLocation::Inside:
            return "inside";
        case Boolean4FacetLocation::OnBoundary:
            return "on-boundary";
    }
    return "invalid facet location";
}


const char *
Boolean4BoundaryFacingName(Boolean4BoundaryFacing facing) {
    switch(facing) {
        case Boolean4BoundaryFacing::None:
            return "none";
        case Boolean4BoundaryFacing::SameFacing:
            return "same-facing";
        case Boolean4BoundaryFacing::OppositeFacing:
            return "opposite-facing";
    }
    return "invalid boundary facing";
}


[[maybe_unused]] std::string
Boolean4DiagnosticString(const Boolean4ValidationDiagnostic &diag) {
    std::ostringstream os;
    os << "Boolean4 " << diag.stage
       << " diagnostic operand=" << Boolean4OperandName(diag.operand)
       << " face=" << diag.face_id
       << " vertex=" << diag.vertex_id
       << ": " << diag.message;
    return os.str();
}


[[maybe_unused]] std::string
Boolean4DiagnosticString(const Boolean4TrianglePairDiagnostic &diag) {
    std::ostringstream os;
    os << "Boolean4 triangle-pair diagnostic lhs_face=" << diag.lhs_face_id
       << " rhs_face=" << diag.rhs_face_id
       << " predicate_case=\"" << Boolean4TriangleTriangleCaseName(diag.predicate_case) << '"'
       << " constructed_vertices=" << diag.constructed_vertex_count;
    return os.str();
}


[[maybe_unused]] std::string
Boolean4DiagnosticString(const Boolean4ArrangementSplitDiagnostic &diag) {
    std::ostringstream os;
    os << "Boolean4 arrangement-split diagnostic operand=" << Boolean4OperandName(diag.source_face.operand)
       << " source_face=" << diag.source_face.face_id
       << " constraints=" << diag.constraint_count
       << " vertices=" << diag.vertex_count
       << " facets=" << diag.facet_count;
    return os.str();
}


[[maybe_unused]] std::string
Boolean4DiagnosticString(const Boolean4FacetClassificationDiagnostic &diag) {
    std::ostringstream os;
    os << "Boolean4 facet-classification diagnostic operand=" << Boolean4OperandName(diag.operand)
       << " split_face=" << diag.split_face_id
       << " facet=" << diag.facet_id
       << " source_face=" << diag.source_face.face_id
       << " location=" << Boolean4FacetLocationName(diag.location)
       << " boundary_facing=" << Boolean4BoundaryFacingName(diag.boundary_facing);
    return os.str();
}


[[maybe_unused]] std::string
Boolean4DiagnosticString(const Boolean4OutputAssemblyDiagnostic &diag) {
    std::ostringstream os;
    os << "Boolean4 output-assembly diagnostic operand=" << Boolean4OperandName(diag.operand)
       << " source_face=" << diag.source_face_id
       << " output_face=" << diag.output_face_id
       << " output_vertex=" << diag.output_vertex_id
       << ": " << diag.message;
    return os.str();
}


template <class T>
struct Boolean4ConstructedPoint {
    vec3<T> approximate;
    Boolean4SnapCoord key;
};


template <class T>
struct Boolean4TriangleTriangleClassification {
    Boolean4TriangleTriangleCase kind = Boolean4TriangleTriangleCase::Disjoint;
    std::vector<Boolean4ConstructedPoint<T>> constraint_vertices;
};


template <class T>
Boolean4SnapCoord
Boolean4SnapKey(const vec3<T> &p) {
    constexpr long double grid_scale = 1000000000000.0L;
    const auto snap_one = [](T v) -> int64_t {
        const long double scaled = static_cast<long double>(v) * grid_scale;
        if(scaled > static_cast<long double>(std::numeric_limits<int64_t>::max())) {
            return std::numeric_limits<int64_t>::max();
        }
        if(scaled < static_cast<long double>(std::numeric_limits<int64_t>::min())) {
            return std::numeric_limits<int64_t>::min();
        }
        return static_cast<int64_t>(std::llround(scaled));
    };
    return { snap_one(p.x), snap_one(p.y), snap_one(p.z), -40 };
}


template <class T>
void
Boolean4AddUniqueConstructedPoint(std::vector<Boolean4ConstructedPoint<T>> &points,
                                  const vec3<T> &p) {
    const auto key = Boolean4SnapKey(p);
    for(const auto &existing : points) {
        if(!(existing.key < key) && !(key < existing.key)) {
            return;
        }
    }
    points.push_back({ p, key });
    std::sort(points.begin(), points.end(), [](const auto &lhs, const auto &rhs) {
        return lhs.key < rhs.key;
    });
}


template <class T>
bool
Boolean4AllSameNonZeroSign(const std::array<int, 3> &signs) {
    return std::all_of(signs.begin(), signs.end(), [](int s) { return s > 0; })
        || std::all_of(signs.begin(), signs.end(), [](int s) { return s < 0; });
}


bool
Boolean4HasBothSigns(const std::array<int, 3> &signs) {
    return std::any_of(signs.begin(), signs.end(), [](int s) { return s < 0; })
        && std::any_of(signs.begin(), signs.end(), [](int s) { return s > 0; });
}


template <class T>
vec3<T>
Boolean4PlaneEdgeIntersection(const vec3<T> &a,
                              const vec3<T> &b,
                              const vec3<T> &p0,
                              const vec3<T> &p1,
                              const vec3<T> &p2) {
    const auto n = (p1 - p0).Cross(p2 - p0);
    const T da = n.Dot(a - p0);
    const T db = n.Dot(b - p0);
    const T denom = da - db;
    if(denom == static_cast<T>(0)) {
        return (a + b) * static_cast<T>(0.5);
    }
    return a + (b - a) * (da / denom);
}


template <class T>
std::vector<Boolean4ConstructedPoint<T>>
Boolean4TrianglePlaneSection(const std::array<vec3<T>, 3> &tri,
                             const std::array<int, 3> &signs,
                             const std::array<vec3<T>, 3> &plane_tri) {
    std::vector<Boolean4ConstructedPoint<T>> points;
    for(size_t i = 0; i < 3UL; ++i) {
        const size_t j = (i + 1UL) % 3UL;
        if(signs[i] == 0) {
            Boolean4AddUniqueConstructedPoint(points, tri[i]);
        }
        if(signs[i] * signs[j] < 0) {
            Boolean4AddUniqueConstructedPoint(points,
                Boolean4PlaneEdgeIntersection(tri[i], tri[j], plane_tri[0], plane_tri[1], plane_tri[2]));
        }
    }
    return points;
}


template <class T>
int
Boolean4DominantProjectionAxis(const std::array<vec3<T>, 3> &tri) {
    const auto n = (tri[1] - tri[0]).Cross(tri[2] - tri[0]);
    const T ax = std::abs(n.x);
    const T ay = std::abs(n.y);
    const T az = std::abs(n.z);
    if(ax >= ay && ax >= az) {
        return 0;
    }
    if(ay >= ax && ay >= az) {
        return 1;
    }
    return 2;
}


template <class T>
vec2<T>
Boolean4ProjectPoint(const vec3<T> &p, int axis) {
    if(axis == 0) {
        return vec2<T>(p.y, p.z);
    }
    if(axis == 1) {
        return vec2<T>(p.x, p.z);
    }
    return vec2<T>(p.x, p.y);
}


template <class T>
std::array<vec2<T>, 3>
Boolean4ProjectTriangle(const std::array<vec3<T>, 3> &tri, int axis) {
    return { Boolean4ProjectPoint(tri[0], axis),
             Boolean4ProjectPoint(tri[1], axis),
             Boolean4ProjectPoint(tri[2], axis) };
}


template <class T>
struct Boolean4ProjectedVertex {
    vec2<T> p2;
    vec3<T> p3;
};


template <class T>
T
Boolean4Cross2(const vec2<T> &a,
               const vec2<T> &b) {
    return a.x * b.y - a.y * b.x;
}


template <class T>
T
Boolean4SignedPolygonArea2(const std::vector<Boolean4ProjectedVertex<T>> &polygon) {
    T area2 = static_cast<T>(0);
    for(size_t i = 0; i < polygon.size(); ++i) {
        const auto &a = polygon[i].p2;
        const auto &b = polygon[(i + 1UL) % polygon.size()].p2;
        area2 += a.x * b.y - a.y * b.x;
    }
    return area2;
}


template <class T>
bool
Boolean4SameProjectedVertex(const Boolean4ProjectedVertex<T> &lhs,
                            const Boolean4ProjectedVertex<T> &rhs) {
    const auto lhs_key = Boolean4SnapKey(lhs.p3);
    const auto rhs_key = Boolean4SnapKey(rhs.p3);
    return !(lhs_key < rhs_key) && !(rhs_key < lhs_key);
}


template <class T>
void
Boolean4PushCleanVertex(std::vector<Boolean4ProjectedVertex<T>> &polygon,
                        const Boolean4ProjectedVertex<T> &vertex) {
    if(!polygon.empty() && Boolean4SameProjectedVertex(polygon.back(), vertex)) {
        return;
    }
    polygon.push_back(vertex);
}


template <class T>
std::vector<Boolean4ProjectedVertex<T>>
Boolean4CleanPolygon(std::vector<Boolean4ProjectedVertex<T>> polygon) {
    std::vector<Boolean4ProjectedVertex<T>> cleaned;
    for(const auto &vertex : polygon) {
        Boolean4PushCleanVertex(cleaned, vertex);
    }
    if(cleaned.size() > 1UL && Boolean4SameProjectedVertex(cleaned.front(), cleaned.back())) {
        cleaned.pop_back();
    }
    if(cleaned.size() < 3UL || std::abs(Boolean4SignedPolygonArea2(cleaned)) == static_cast<T>(0)) {
        return {};
    }
    return cleaned;
}


template <class T>
Boolean4ProjectedVertex<T>
Boolean4LineIntersectionVertex(const Boolean4ProjectedVertex<T> &u,
                               const Boolean4ProjectedVertex<T> &v,
                               const vec2<T> &a,
                               const vec2<T> &b) {
    const auto edge = v.p2 - u.p2;
    const auto line = b - a;
    const T denom = Boolean4Cross2(edge, line);
    if(denom == static_cast<T>(0)) {
        return { (u.p2 + v.p2) * static_cast<T>(0.5),
                 (u.p3 + v.p3) * static_cast<T>(0.5) };
    }
    const T t = Boolean4Cross2(a - u.p2, line) / denom;
    return { u.p2 + edge * t, u.p3 + (v.p3 - u.p3) * t };
}


template <class T>
std::vector<Boolean4ProjectedVertex<T>>
Boolean4ClipPolygonByLineSide(const std::vector<Boolean4ProjectedVertex<T>> &polygon,
                              const vec2<T> &a,
                              const vec2<T> &b,
                              int keep_sign) {
    std::vector<Boolean4ProjectedVertex<T>> clipped;
    for(size_t i = 0; i < polygon.size(); ++i) {
        const auto &u = polygon[i];
        const auto &v = polygon[(i + 1UL) % polygon.size()];
        const int su = orient_sign(a, b, u.p2);
        const int sv = orient_sign(a, b, v.p2);
        const bool u_inside = (keep_sign > 0) ? (su >= 0) : (su <= 0);
        const bool v_inside = (keep_sign > 0) ? (sv >= 0) : (sv <= 0);

        if(u_inside) {
            Boolean4PushCleanVertex(clipped, u);
        }
        if(u_inside != v_inside) {
            Boolean4PushCleanVertex(clipped, Boolean4LineIntersectionVertex(u, v, a, b));
        }
    }
    return Boolean4CleanPolygon(std::move(clipped));
}


template <class T>
void
Boolean4SplitPolygonsBySegmentLine(std::vector<std::vector<Boolean4ProjectedVertex<T>>> &polygons,
                                   const Boolean4ProjectedVertex<T> &a,
                                   const Boolean4ProjectedVertex<T> &b) {
    if(Boolean4SameProjectedVertex(a, b)) {
        return;
    }

    std::vector<std::vector<Boolean4ProjectedVertex<T>>> split;
    for(const auto &polygon : polygons) {
        bool has_positive = false;
        bool has_negative = false;
        for(const auto &vertex : polygon) {
            const int s = orient_sign(a.p2, b.p2, vertex.p2);
            has_positive = has_positive || s > 0;
            has_negative = has_negative || s < 0;
        }
        if(!has_positive || !has_negative) {
            split.push_back(polygon);
            continue;
        }

        auto positive = Boolean4ClipPolygonByLineSide(polygon, a.p2, b.p2, 1);
        auto negative = Boolean4ClipPolygonByLineSide(polygon, a.p2, b.p2, -1);
        if(positive.empty() || negative.empty()) {
            split.push_back(polygon);
            continue;
        }
        split.push_back(std::move(positive));
        split.push_back(std::move(negative));
    }

    std::sort(split.begin(), split.end(), [](const auto &lhs, const auto &rhs) {
        const auto lhs_area = std::abs(Boolean4SignedPolygonArea2(lhs));
        const auto rhs_area = std::abs(Boolean4SignedPolygonArea2(rhs));
        if(lhs_area != rhs_area) {
            return lhs_area < rhs_area;
        }
        return Boolean4SnapKey(lhs.front().p3) < Boolean4SnapKey(rhs.front().p3);
    });
    polygons = std::move(split);
}


template <class T>
uint64_t
Boolean4ArrangementVertexId(Boolean4SplitTriangleResult<T> &result,
                            const vec3<T> &p,
                            const Boolean4SourceFaceRef &source_face) {
    const auto key = Boolean4SnapKey(p);
    for(size_t i = 0; i < result.vertices.size(); ++i) {
        const auto &existing = result.vertices[i].key;
        if(!(existing < key) && !(key < existing)) {
            return static_cast<uint64_t>(i);
        }
    }

    const uint64_t id = static_cast<uint64_t>(result.vertices.size());
    Boolean4VertexRecord vertex;
    vertex.stable_id = id;
    vertex.kind = Boolean4VertexKind::CoplanarConstraintVertex;
    vertex.key = key;
    vertex.source_faces.push_back(source_face);
    result.vertices.push_back(std::move(vertex));
    result.approximate_vertices.push_back(p);
    return id;
}


template <class T>
Boolean4SplitTriangleResult<T>
SplitBoolean4TriangleIntoArrangementFacets(const std::array<vec3<T>, 3> &triangle,
                                           const Boolean4SourceFaceRef &source_face,
                                           std::vector<Boolean4ConstraintSegment<T>> constraints) {
    const int axis = Boolean4DominantProjectionAxis(triangle);
    std::sort(constraints.begin(), constraints.end());

    std::vector<std::vector<Boolean4ProjectedVertex<T>>> polygons = { {
        { Boolean4ProjectPoint(triangle[0], axis), triangle[0] },
        { Boolean4ProjectPoint(triangle[1], axis), triangle[1] },
        { Boolean4ProjectPoint(triangle[2], axis), triangle[2] } } };

    for(const auto &constraint : constraints) {
        const Boolean4ProjectedVertex<T> a = { Boolean4ProjectPoint(constraint.a, axis), constraint.a };
        const Boolean4ProjectedVertex<T> b = { Boolean4ProjectPoint(constraint.b, axis), constraint.b };
        Boolean4SplitPolygonsBySegmentLine(polygons, a, b);
    }

    Boolean4SplitTriangleResult<T> result;
    for(const auto &polygon : polygons) {
        if(polygon.size() < 3UL) {
            continue;
        }
        const auto v0 = Boolean4ArrangementVertexId(result, polygon[0].p3, source_face);
        for(size_t i = 1; i + 1UL < polygon.size(); ++i) {
            const auto v1 = Boolean4ArrangementVertexId(result, polygon[i].p3, source_face);
            const auto v2 = Boolean4ArrangementVertexId(result, polygon[i + 1UL].p3, source_face);
            if(v0 == v1 || v1 == v2 || v2 == v0) {
                continue;
            }
            Boolean4ArrangementFacet<T> facet;
            facet.stable_id = static_cast<uint64_t>(result.facets.size());
            facet.source_face = source_face;
            facet.vertex_ids = { { v0, v1, v2 } };
            result.facets.push_back(facet);
        }
    }

    std::sort(result.facets.begin(), result.facets.end());
    for(size_t i = 0; i < result.facets.size(); ++i) {
        result.facets[i].stable_id = static_cast<uint64_t>(i);
    }
    return result;
}


template <class T>
bool
Boolean4SamePoint2(const vec2<T> &a, const vec2<T> &b) {
    return a.x == b.x && a.y == b.y;
}


template <class T>
bool
Boolean4SameVertexSet2(const std::array<vec2<T>, 3> &lhs,
                       const std::array<vec2<T>, 3> &rhs) {
    std::array<bool, 3> used = { false, false, false };
    for(const auto &p : lhs) {
        bool found = false;
        for(size_t i = 0; i < 3UL; ++i) {
            if(!used[i] && Boolean4SamePoint2(p, rhs[i])) {
                used[i] = true;
                found = true;
                break;
            }
        }
        if(!found) {
            return false;
        }
    }
    return true;
}


template <class T>
bool
Boolean4StrictlyInsideTriangle2(const vec2<T> &p,
                                const std::array<vec2<T>, 3> &tri) {
    const int o = orient_sign(tri[0], tri[1], tri[2]);
    if(o == 0) {
        return false;
    }
    const int o0 = orient_sign(tri[0], tri[1], p);
    const int o1 = orient_sign(tri[1], tri[2], p);
    const int o2 = orient_sign(tri[2], tri[0], p);
    return (o > 0) ? (o0 > 0 && o1 > 0 && o2 > 0)
                   : (o0 < 0 && o1 < 0 && o2 < 0);
}


template <class T>
bool
Boolean4CollinearSegmentOverlap2(const vec2<T> &a,
                                 const vec2<T> &b,
                                 const vec2<T> &c,
                                 const vec2<T> &d) {
    if(orient_sign(a, b, c) != 0 || orient_sign(a, b, d) != 0) {
        return false;
    }
    const bool use_x = std::abs(b.x - a.x) >= std::abs(b.y - a.y);
    const T a0 = use_x ? a.x : a.y;
    const T b0 = use_x ? b.x : b.y;
    const T c0 = use_x ? c.x : c.y;
    const T d0 = use_x ? d.x : d.y;
    return std::max(std::min(a0, b0), std::min(c0, d0))
         < std::min(std::max(a0, b0), std::max(c0, d0));
}


template <class T>
Boolean4TriangleTriangleClassification<T>
ClassifyBoolean4TriangleTriangle(const std::array<vec3<T>, 3> &lhs,
                                 const std::array<vec3<T>, 3> &rhs) {
    const std::array<int, 3> rhs_against_lhs = {
        orient_sign(lhs[0], lhs[1], lhs[2], rhs[0]),
        orient_sign(lhs[0], lhs[1], lhs[2], rhs[1]),
        orient_sign(lhs[0], lhs[1], lhs[2], rhs[2]) };
    const std::array<int, 3> lhs_against_rhs = {
        orient_sign(rhs[0], rhs[1], rhs[2], lhs[0]),
        orient_sign(rhs[0], rhs[1], rhs[2], lhs[1]),
        orient_sign(rhs[0], rhs[1], rhs[2], lhs[2]) };

    if(Boolean4AllSameNonZeroSign<T>(rhs_against_lhs)
    || Boolean4AllSameNonZeroSign<T>(lhs_against_rhs)) {
        return { Boolean4TriangleTriangleCase::Disjoint, {} };
    }

    const bool coplanar = std::all_of(rhs_against_lhs.begin(), rhs_against_lhs.end(), [](int s) { return s == 0; })
                       && std::all_of(lhs_against_rhs.begin(), lhs_against_rhs.end(), [](int s) { return s == 0; });
    if(coplanar) {
        const int axis = Boolean4DominantProjectionAxis(lhs);
        const auto lhs2 = Boolean4ProjectTriangle(lhs, axis);
        const auto rhs2 = Boolean4ProjectTriangle(rhs, axis);
        if(Boolean4SameVertexSet2(lhs2, rhs2)) {
            const auto lhs_n = (lhs[1] - lhs[0]).Cross(lhs[2] - lhs[0]);
            const auto rhs_n = (rhs[1] - rhs[0]).Cross(rhs[2] - rhs[0]);
            return { lhs_n.Dot(rhs_n) >= static_cast<T>(0)
                   ? Boolean4TriangleTriangleCase::IdenticalTriangles
                   : Boolean4TriangleTriangleCase::ReversedIdenticalTriangles,
                     { { lhs[0], Boolean4SnapKey(lhs[0]) },
                       { lhs[1], Boolean4SnapKey(lhs[1]) },
                       { lhs[2], Boolean4SnapKey(lhs[2]) } } };
        }

        bool area_overlap = false;
        bool segment_overlap = false;
        bool point_contact = false;
        for(size_t i = 0; i < 3UL; ++i) {
            area_overlap = area_overlap
                         || Boolean4StrictlyInsideTriangle2(lhs2[i], rhs2)
                         || Boolean4StrictlyInsideTriangle2(rhs2[i], lhs2);
            for(size_t j = 0; j < 3UL; ++j) {
                const auto &a = lhs2[i];
                const auto &b = lhs2[(i + 1UL) % 3UL];
                const auto &c = rhs2[j];
                const auto &d = rhs2[(j + 1UL) % 3UL];
                if(Boolean4CollinearSegmentOverlap2(a, b, c, d)) {
                    segment_overlap = true;
                }
                if(segments_intersect_beyond_shared_endpoints(a, b, c, d)) {
                    area_overlap = true;
                }
                if(point_on_closed_segment(a, c, d)
                || point_on_closed_segment(b, c, d)
                || point_on_closed_segment(c, a, b)
                || point_on_closed_segment(d, a, b)) {
                    point_contact = true;
                }
            }
        }

        if(area_overlap) {
            return { Boolean4TriangleTriangleCase::CoplanarOverlapArea, {} };
        }
        if(segment_overlap) {
            return { Boolean4TriangleTriangleCase::SharedEdgeOrPartialOverlappingEdge, {} };
        }
        if(point_contact) {
            return { Boolean4TriangleTriangleCase::PointContact, {} };
        }
        return { Boolean4TriangleTriangleCase::CoplanarDisjoint, {} };
    }

    const int lhs_axis = Boolean4DominantProjectionAxis(lhs);
    const int rhs_axis = Boolean4DominantProjectionAxis(rhs);
    const auto lhs2 = Boolean4ProjectTriangle(lhs, lhs_axis);
    const auto rhs2 = Boolean4ProjectTriangle(rhs, rhs_axis);
    if(!Boolean4HasBothSigns(rhs_against_lhs) && !Boolean4HasBothSigns(lhs_against_rhs)) {
        std::vector<Boolean4ConstructedPoint<T>> points;
        for(const auto &p : lhs) {
            if(orient_sign(rhs[0], rhs[1], rhs[2], p) == 0
            && point_in_triangle_or_on_boundary(Boolean4ProjectPoint(p, rhs_axis), rhs2[0], rhs2[1], rhs2[2])) {
                Boolean4AddUniqueConstructedPoint(points, p);
            }
        }
        for(const auto &p : rhs) {
            if(orient_sign(lhs[0], lhs[1], lhs[2], p) == 0
            && point_in_triangle_or_on_boundary(Boolean4ProjectPoint(p, lhs_axis), lhs2[0], lhs2[1], lhs2[2])) {
                Boolean4AddUniqueConstructedPoint(points, p);
            }
        }
        if(points.size() >= 2UL) {
            return { Boolean4TriangleTriangleCase::SharedEdgeOrPartialOverlappingEdge, points };
        }
        if(points.size() == 1UL) {
            return { Boolean4TriangleTriangleCase::PointContact, points };
        }
        return { Boolean4TriangleTriangleCase::Disjoint, {} };
    }

    std::vector<Boolean4ConstructedPoint<T>> points;
    for(const auto &p : Boolean4TrianglePlaneSection(lhs, lhs_against_rhs, rhs)) {
        Boolean4AddUniqueConstructedPoint(points, p.approximate);
    }
    for(const auto &p : Boolean4TrianglePlaneSection(rhs, rhs_against_lhs, lhs)) {
        Boolean4AddUniqueConstructedPoint(points, p.approximate);
    }
    if(points.size() <= 1UL) {
        return { Boolean4TriangleTriangleCase::PointContact, points };
    }
    return { Boolean4TriangleTriangleCase::ProperSegmentIntersection, points };
}


template <class T, class I>
struct Boolean4NormalizedInput {
    fv_surface_mesh<T, I> mesh;
    std::vector<Boolean4SourceFaceRef> source_faces;
};


template <class T, class I>
T
Boolean4SignedFaceVolume(const fv_surface_mesh<T, I> &mesh,
                         I face_id) {
    const auto &face = mesh.faces.at(face_id);
    const auto &a = mesh.vertices.at(face.at(0));
    const auto &b = mesh.vertices.at(face.at(1));
    const auto &c = mesh.vertices.at(face.at(2));
    return a.Dot(b.Cross(c)) / static_cast<T>(6);
}


template <class T, class I>
void
RequireNonZeroComponentVolumes4(const fv_surface_mesh<T, I> &mesh,
                                const char *operand_name) {
    std::vector<std::vector<I>> vertex_faces(mesh.vertices.size());
    for(I f = 0; f < static_cast<I>(mesh.faces.size()); ++f) {
        for(const auto v : mesh.faces[f]) {
            vertex_faces[v].push_back(f);
        }
    }

    std::vector<bool> visited(mesh.faces.size(), false);
    for(I start = 0; start < static_cast<I>(mesh.faces.size()); ++start) {
        if(visited[start]) {
            continue;
        }

        T component_volume = static_cast<T>(0);
        std::vector<I> stack = { start };
        visited[start] = true;
        while(!stack.empty()) {
            const I face_id = stack.back();
            stack.pop_back();
            component_volume += Boolean4SignedFaceVolume(mesh, face_id);

            for(const auto v : mesh.faces[face_id]) {
                for(const auto adjacent_face_id : vertex_faces[v]) {
                    if(!visited[adjacent_face_id]) {
                        visited[adjacent_face_id] = true;
                        stack.push_back(adjacent_face_id);
                    }
                }
            }
        }

        if(std::abs(component_volume) == static_cast<T>(0)) {
            throw std::invalid_argument(std::string("BooleanMeshOp4 ")
                                      + operand_name
                                      + " operand has a zero-volume closed component");
        }
    }
}


template <class T, class I>
Boolean4NormalizedInput<T, I>
NormalizeAndValidateInput4(const fv_surface_mesh<T, I> &input,
                           const char *operand_name,
                           Boolean4Operand operand) {
    if(!HasOnlyFiniteVertices(input)) {
        throw std::invalid_argument(std::string("BooleanMeshOp4 ")
                                  + operand_name
                                  + " operand contains a non-finite vertex");
    }

    if(input.faces.empty() || input.vertices.empty()) {
        return Boolean4NormalizedInput<T, I>();
    }

    if(!HasValidFaceIndices(input)) {
        throw std::invalid_argument(std::string("BooleanMeshOp4 ")
                                  + operand_name
                                  + " operand contains an out-of-range face index");
    }

    Boolean4NormalizedInput<T, I> normalized;
    normalized.mesh.vertices = input.vertices;
    normalized.source_faces.reserve(input.faces.size());

    for(uint64_t source_face_id = 0; source_face_id < input.faces.size(); ++source_face_id) {
        const auto &face = input.faces[source_face_id];
        if(face.size() < 3UL) {
            throw std::invalid_argument(std::string("BooleanMeshOp4 ")
                                      + operand_name
                                      + " operand contains a face with fewer than three vertices");
        }

        if(face.size() == 3UL) {
            normalized.mesh.faces.push_back(face);
            normalized.source_faces.push_back({ operand, source_face_id });
            continue;
        }

        for(size_t i = 0; i <= face.size() - 3UL; ++i) {
            normalized.mesh.faces.push_back({ face[0], face[i + 1UL], face[i + 2UL] });
            normalized.source_faces.push_back({ operand, source_face_id });
        }
    }

    normalized.mesh.recreate_involved_face_index();

    if(!IsTriangularMesh(normalized.mesh)) {
        throw std::invalid_argument(std::string("BooleanMeshOp4 ")
                                  + operand_name
                                  + " operand could not be normalized to triangles");
    }
    if(!HasValidFaceIndices(normalized.mesh)) {
        throw std::invalid_argument(std::string("BooleanMeshOp4 ")
                                  + operand_name
                                  + " operand contains an out-of-range normalized face index");
    }
    if(!HasNoDegenerateFaces(normalized.mesh)) {
        throw std::invalid_argument(std::string("BooleanMeshOp4 ")
                                  + operand_name
                                  + " operand contains a degenerate triangle");
    }
    if(!IsClosedManifold(normalized.mesh)) {
        throw std::invalid_argument(std::string("BooleanMeshOp4 ")
                                  + operand_name
                                  + " operand is not a closed manifold mesh");
    }
    if(!HasConsistentOrientation(normalized.mesh)) {
        throw std::invalid_argument(std::string("BooleanMeshOp4 ")
                                  + operand_name
                                  + " operand has inconsistent face orientation");
    }

    RequireNonZeroComponentVolumes4(normalized.mesh, operand_name);
    return normalized;
}


template <class T, class I>
bool
Boolean4DetectAxisAlignedCuboid(const Boolean4NormalizedInput<T, I> &input,
                                Boolean4Cuboid<T> &cuboid) {
    if(input.mesh.vertices.empty() || input.mesh.faces.empty()) {
        return false;
    }

    cuboid.xmin = cuboid.xmax = input.mesh.vertices.front().x;
    cuboid.ymin = cuboid.ymax = input.mesh.vertices.front().y;
    cuboid.zmin = cuboid.zmax = input.mesh.vertices.front().z;
    for(const auto &v : input.mesh.vertices) {
        cuboid.xmin = std::min(cuboid.xmin, v.x);
        cuboid.ymin = std::min(cuboid.ymin, v.y);
        cuboid.zmin = std::min(cuboid.zmin, v.z);
        cuboid.xmax = std::max(cuboid.xmax, v.x);
        cuboid.ymax = std::max(cuboid.ymax, v.y);
        cuboid.zmax = std::max(cuboid.zmax, v.z);
    }
    if(cuboid.xmin == cuboid.xmax || cuboid.ymin == cuboid.ymax || cuboid.zmin == cuboid.zmax) {
        return false;
    }

    size_t boundary_vertex_count = 0;
    for(const auto &v : input.mesh.vertices) {
        const bool on_x = v.x == cuboid.xmin || v.x == cuboid.xmax;
        const bool on_y = v.y == cuboid.ymin || v.y == cuboid.ymax;
        const bool on_z = v.z == cuboid.zmin || v.z == cuboid.zmax;
        if(!on_x || !on_y || !on_z) {
            return false;
        }
        ++boundary_vertex_count;
    }
    if(boundary_vertex_count != 8UL || input.mesh.faces.size() != 12UL) {
        return false;
    }

    std::array<size_t, 6> face_counts = { { 0, 0, 0, 0, 0, 0 } };
    for(const auto &face : input.mesh.faces) {
        const auto &a = input.mesh.vertices.at(face.at(0));
        const auto &b = input.mesh.vertices.at(face.at(1));
        const auto &c = input.mesh.vertices.at(face.at(2));
        if(a.x == cuboid.xmin && b.x == cuboid.xmin && c.x == cuboid.xmin) {
            ++face_counts[0];
        } else if(a.x == cuboid.xmax && b.x == cuboid.xmax && c.x == cuboid.xmax) {
            ++face_counts[1];
        } else if(a.y == cuboid.ymin && b.y == cuboid.ymin && c.y == cuboid.ymin) {
            ++face_counts[2];
        } else if(a.y == cuboid.ymax && b.y == cuboid.ymax && c.y == cuboid.ymax) {
            ++face_counts[3];
        } else if(a.z == cuboid.zmin && b.z == cuboid.zmin && c.z == cuboid.zmin) {
            ++face_counts[4];
        } else if(a.z == cuboid.zmax && b.z == cuboid.zmax && c.z == cuboid.zmax) {
            ++face_counts[5];
        } else {
            return false;
        }
    }
    return std::all_of(face_counts.begin(), face_counts.end(), [](size_t count) { return count == 2UL; });
}


template <class T>
std::vector<T>
Boolean4SortedUniqueCoords(std::initializer_list<T> coords) {
    std::vector<T> result(coords.begin(), coords.end());
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}


template <class T>
bool
Boolean4CellInsideCuboid(const std::vector<T> &xs,
                         const std::vector<T> &ys,
                         const std::vector<T> &zs,
                         size_t ix,
                         size_t iy,
                         size_t iz,
                         const Boolean4Cuboid<T> &cuboid) {
    const T x = (xs[ix] + xs[ix + 1UL]) * static_cast<T>(0.5);
    const T y = (ys[iy] + ys[iy + 1UL]) * static_cast<T>(0.5);
    const T z = (zs[iz] + zs[iz + 1UL]) * static_cast<T>(0.5);
    return cuboid.xmin < x && x < cuboid.xmax
        && cuboid.ymin < y && y < cuboid.ymax
        && cuboid.zmin < z && z < cuboid.zmax;
}


inline size_t
Boolean4CellIndex(size_t ix,
                  size_t iy,
                  size_t iz,
                  size_t ny,
                  size_t nz) {
    return (ix * ny + iy) * nz + iz;
}


template <class T>
bool
Boolean4ApplyCellTruthTable(bool lhs_inside,
                            bool rhs_inside,
                            MeshBooleanOperation4 op) {
    switch(op) {
        case MeshBooleanOperation4::Union:
            return lhs_inside || rhs_inside;
        case MeshBooleanOperation4::Intersection:
            return lhs_inside && rhs_inside;
        case MeshBooleanOperation4::Exclusion:
            return lhs_inside != rhs_inside;
        case MeshBooleanOperation4::Subtraction:
            return lhs_inside && !rhs_inside;
    }
    return false;
}


template <class T, class I>
I
Boolean4CuboidOutputVertex(fv_surface_mesh<T, I> &mesh,
                           std::vector<std::tuple<size_t, size_t, size_t, size_t, I>> &vertices,
                           size_t component_id,
                           size_t ix,
                           size_t iy,
                           size_t iz,
                           const std::vector<T> &xs,
                           const std::vector<T> &ys,
                           const std::vector<T> &zs) {
    for(const auto &entry : vertices) {
        if(std::get<0>(entry) == component_id
        && std::get<1>(entry) == ix
        && std::get<2>(entry) == iy
        && std::get<3>(entry) == iz) {
            return std::get<4>(entry);
        }
    }
    const auto id = Boolean4CheckedIndex<I>(static_cast<uint64_t>(mesh.vertices.size()), "vertices");
    mesh.vertices.push_back(vec3<T>(xs[ix], ys[iy], zs[iz]));
    vertices.push_back(std::make_tuple(component_id, ix, iy, iz, id));
    return id;
}


template <class T, class I>
void
Boolean4AppendCuboidBoundaryQuad(fv_surface_mesh<T, I> &mesh,
                                 std::vector<std::tuple<size_t, size_t, size_t, size_t, I>> &vertices,
                                 size_t component_id,
                                 const std::array<std::array<size_t, 3>, 4> &corners,
                                 const std::vector<T> &xs,
                                 const std::vector<T> &ys,
                                 const std::vector<T> &zs) {
    const I a = Boolean4CuboidOutputVertex(mesh, vertices, component_id, corners[0][0], corners[0][1], corners[0][2], xs, ys, zs);
    const I b = Boolean4CuboidOutputVertex(mesh, vertices, component_id, corners[1][0], corners[1][1], corners[1][2], xs, ys, zs);
    const I c = Boolean4CuboidOutputVertex(mesh, vertices, component_id, corners[2][0], corners[2][1], corners[2][2], xs, ys, zs);
    const I d = Boolean4CuboidOutputVertex(mesh, vertices, component_id, corners[3][0], corners[3][1], corners[3][2], xs, ys, zs);
    mesh.faces.push_back({ a, b, c });
    mesh.faces.push_back({ a, c, d });
}


template <class T, class I>
fv_surface_mesh<T, I>
RunBoolean4CuboidCellArrangement(const Boolean4Cuboid<T> &lhs,
                                 const Boolean4Cuboid<T> &rhs,
                                 MeshBooleanOperation4 op) {
    const auto xs = Boolean4SortedUniqueCoords<T>({ lhs.xmin, lhs.xmax, rhs.xmin, rhs.xmax });
    const auto ys = Boolean4SortedUniqueCoords<T>({ lhs.ymin, lhs.ymax, rhs.ymin, rhs.ymax });
    const auto zs = Boolean4SortedUniqueCoords<T>({ lhs.zmin, lhs.zmax, rhs.zmin, rhs.zmax });
    const size_t nx = xs.size() - 1UL;
    const size_t ny = ys.size() - 1UL;
    const size_t nz = zs.size() - 1UL;

    std::vector<bool> selected(nx * ny * nz, false);
    for(size_t ix = 0; ix < nx; ++ix) {
        for(size_t iy = 0; iy < ny; ++iy) {
            for(size_t iz = 0; iz < nz; ++iz) {
                const bool lhs_inside = Boolean4CellInsideCuboid(xs, ys, zs, ix, iy, iz, lhs);
                const bool rhs_inside = Boolean4CellInsideCuboid(xs, ys, zs, ix, iy, iz, rhs);
                selected[Boolean4CellIndex(ix, iy, iz, ny, nz)] = Boolean4ApplyCellTruthTable<T>(lhs_inside, rhs_inside, op);
            }
        }
    }
    if(std::none_of(selected.begin(), selected.end(), [](bool v) { return v; })) {
        return fv_surface_mesh<T, I>();
    }

    std::vector<size_t> component(selected.size(), std::numeric_limits<size_t>::max());
    size_t component_count = 0;
    for(size_t start = 0; start < selected.size(); ++start) {
        if(!selected[start] || component[start] != std::numeric_limits<size_t>::max()) {
            continue;
        }
        std::vector<size_t> stack = { start };
        component[start] = component_count;
        while(!stack.empty()) {
            const size_t cur = stack.back();
            stack.pop_back();
            const size_t ix = cur / (ny * nz);
            const size_t rem = cur % (ny * nz);
            const size_t iy = rem / nz;
            const size_t iz = rem % nz;
            const std::array<std::array<size_t, 3>, 6> neighbors = { {
                { ix > 0UL ? ix - 1UL : nx, iy, iz },
                { ix + 1UL < nx ? ix + 1UL : nx, iy, iz },
                { ix, iy > 0UL ? iy - 1UL : ny, iz },
                { ix, iy + 1UL < ny ? iy + 1UL : ny, iz },
                { ix, iy, iz > 0UL ? iz - 1UL : nz },
                { ix, iy, iz + 1UL < nz ? iz + 1UL : nz } } };
            for(const auto &n : neighbors) {
                if(n[0] >= nx || n[1] >= ny || n[2] >= nz) {
                    continue;
                }
                const size_t ni = Boolean4CellIndex(n[0], n[1], n[2], ny, nz);
                if(selected[ni] && component[ni] == std::numeric_limits<size_t>::max()) {
                    component[ni] = component_count;
                    stack.push_back(ni);
                }
            }
        }
        ++component_count;
    }

    fv_surface_mesh<T, I> mesh;
    std::vector<std::tuple<size_t, size_t, size_t, size_t, I>> vertices;
    for(size_t ix = 0; ix < nx; ++ix) {
        for(size_t iy = 0; iy < ny; ++iy) {
            for(size_t iz = 0; iz < nz; ++iz) {
                const size_t cell = Boolean4CellIndex(ix, iy, iz, ny, nz);
                if(!selected[cell]) {
                    continue;
                }
                const size_t cid = component[cell];
                const auto neighbor_selected = [&](int dx, int dy, int dz) {
                    const int64_t nix = static_cast<int64_t>(ix) + dx;
                    const int64_t niy = static_cast<int64_t>(iy) + dy;
                    const int64_t niz = static_cast<int64_t>(iz) + dz;
                    if(nix < 0 || niy < 0 || niz < 0
                    || nix >= static_cast<int64_t>(nx)
                    || niy >= static_cast<int64_t>(ny)
                    || niz >= static_cast<int64_t>(nz)) {
                        return false;
                    }
                    return static_cast<bool>(selected[Boolean4CellIndex(static_cast<size_t>(nix), static_cast<size_t>(niy), static_cast<size_t>(niz), ny, nz)]);
                };
                if(!neighbor_selected(-1, 0, 0)) {
                    Boolean4AppendCuboidBoundaryQuad(mesh, vertices, cid, { { {ix, iy, iz}, {ix, iy, iz + 1UL}, {ix, iy + 1UL, iz + 1UL}, {ix, iy + 1UL, iz} } }, xs, ys, zs);
                }
                if(!neighbor_selected(1, 0, 0)) {
                    Boolean4AppendCuboidBoundaryQuad(mesh, vertices, cid, { { {ix + 1UL, iy, iz}, {ix + 1UL, iy + 1UL, iz}, {ix + 1UL, iy + 1UL, iz + 1UL}, {ix + 1UL, iy, iz + 1UL} } }, xs, ys, zs);
                }
                if(!neighbor_selected(0, -1, 0)) {
                    Boolean4AppendCuboidBoundaryQuad(mesh, vertices, cid, { { {ix, iy, iz}, {ix + 1UL, iy, iz}, {ix + 1UL, iy, iz + 1UL}, {ix, iy, iz + 1UL} } }, xs, ys, zs);
                }
                if(!neighbor_selected(0, 1, 0)) {
                    Boolean4AppendCuboidBoundaryQuad(mesh, vertices, cid, { { {ix, iy + 1UL, iz}, {ix, iy + 1UL, iz + 1UL}, {ix + 1UL, iy + 1UL, iz + 1UL}, {ix + 1UL, iy + 1UL, iz} } }, xs, ys, zs);
                }
                if(!neighbor_selected(0, 0, -1)) {
                    Boolean4AppendCuboidBoundaryQuad(mesh, vertices, cid, { { {ix, iy, iz}, {ix, iy + 1UL, iz}, {ix + 1UL, iy + 1UL, iz}, {ix + 1UL, iy, iz} } }, xs, ys, zs);
                }
                if(!neighbor_selected(0, 0, 1)) {
                    Boolean4AppendCuboidBoundaryQuad(mesh, vertices, cid, { { {ix, iy, iz + 1UL}, {ix + 1UL, iy, iz + 1UL}, {ix + 1UL, iy + 1UL, iz + 1UL}, {ix, iy + 1UL, iz + 1UL} } }, xs, ys, zs);
                }
            }
        }
    }
    mesh.recreate_involved_face_index();
    VerifyBoolean4OutputPostconditions(mesh, op);
    return mesh;
}


template <class T>
T
Boolean4OutwardLow(T v) {
    return std::nextafter(v, -std::numeric_limits<T>::infinity());
}


template <class T>
T
Boolean4OutwardHigh(T v) {
    return std::nextafter(v, std::numeric_limits<T>::infinity());
}


template <class T>
Boolean4AABB<T>
Boolean4TriangleBounds(const vec3<T> &a,
                       const vec3<T> &b,
                       const vec3<T> &c) {
    const T min_x = std::min(a.x, std::min(b.x, c.x));
    const T min_y = std::min(a.y, std::min(b.y, c.y));
    const T min_z = std::min(a.z, std::min(b.z, c.z));
    const T max_x = std::max(a.x, std::max(b.x, c.x));
    const T max_y = std::max(a.y, std::max(b.y, c.y));
    const T max_z = std::max(a.z, std::max(b.z, c.z));

    return { vec3<T>(Boolean4OutwardLow(min_x),
                     Boolean4OutwardLow(min_y),
                     Boolean4OutwardLow(min_z)),
             vec3<T>(Boolean4OutwardHigh(max_x),
                     Boolean4OutwardHigh(max_y),
                     Boolean4OutwardHigh(max_z)) };
}


template <class T>
bool
Boolean4AABBOverlapInclusive(const Boolean4AABB<T> &lhs,
                             const Boolean4AABB<T> &rhs) {
    return lhs.min.x <= rhs.max.x && rhs.min.x <= lhs.max.x
        && lhs.min.y <= rhs.max.y && rhs.min.y <= lhs.max.y
        && lhs.min.z <= rhs.max.z && rhs.min.z <= lhs.max.z;
}


template <class T, class I>
std::vector<Boolean4TriangleAABB<T>>
BuildBoolean4TriangleAABBs(const Boolean4NormalizedInput<T, I> &input) {
    std::vector<Boolean4TriangleAABB<T>> bounds;
    bounds.reserve(input.mesh.faces.size());

    for(uint64_t face_id = 0; face_id < input.mesh.faces.size(); ++face_id) {
        const auto &face = input.mesh.faces[face_id];
        const auto &a = input.mesh.vertices.at(face.at(0));
        const auto &b = input.mesh.vertices.at(face.at(1));
        const auto &c = input.mesh.vertices.at(face.at(2));
        bounds.push_back({ face_id,
                           input.source_faces.at(face_id),
                           Boolean4TriangleBounds(a, b, c) });
    }

    std::sort(bounds.begin(), bounds.end());
    return bounds;
}


template <class T, class I>
std::vector<Boolean4TrianglePairCandidate>
BuildBoolean4TrianglePairBroadPhase(const Boolean4NormalizedInput<T, I> &lhs,
                                    const Boolean4NormalizedInput<T, I> &rhs) {
    const auto lhs_bounds = BuildBoolean4TriangleAABBs(lhs);
    const auto rhs_bounds = BuildBoolean4TriangleAABBs(rhs);
    std::vector<Boolean4TrianglePairCandidate> candidates;

    for(const auto &lhs_tri : lhs_bounds) {
        for(const auto &rhs_tri : rhs_bounds) {
            if(rhs_tri.bounds.min.x > lhs_tri.bounds.max.x) {
                break;
            }
            if(rhs_tri.bounds.max.x < lhs_tri.bounds.min.x) {
                continue;
            }
            if(Boolean4AABBOverlapInclusive(lhs_tri.bounds, rhs_tri.bounds)) {
                candidates.push_back({ lhs_tri.normalized_face_id,
                                       rhs_tri.normalized_face_id,
                                       lhs_tri.source_face,
                                       rhs_tri.source_face });
            }
        }
    }

    std::sort(candidates.begin(), candidates.end());
    return candidates;
}


template <class T, class I>
std::array<vec3<T>, 3>
Boolean4FaceTriangle(const Boolean4NormalizedInput<T, I> &input,
                     uint64_t face_id) {
    const auto &face = input.mesh.faces.at(face_id);
    return { input.mesh.vertices.at(face.at(0)),
             input.mesh.vertices.at(face.at(1)),
             input.mesh.vertices.at(face.at(2)) };
}


template <class T, class I>
Boolean4OperandArrangement<T>
BuildBoolean4OperandArrangement(const Boolean4NormalizedInput<T, I> &input,
                                const std::vector<std::vector<Boolean4ConstraintSegment<T>>> &constraints_by_face) {
    Boolean4OperandArrangement<T> arrangement;
    arrangement.split_faces.reserve(input.mesh.faces.size());
    for(uint64_t face_id = 0; face_id < input.mesh.faces.size(); ++face_id) {
        arrangement.split_faces.push_back(
            SplitBoolean4TriangleIntoArrangementFacets(Boolean4FaceTriangle(input, face_id),
                                                       input.source_faces.at(face_id),
                                                       constraints_by_face.at(face_id)));
    }
    return arrangement;
}


template <class T, class I>
Boolean4PairArrangement<T>
BuildBoolean4PairArrangement(const Boolean4NormalizedInput<T, I> &lhs,
                             const Boolean4NormalizedInput<T, I> &rhs) {
    std::vector<std::vector<Boolean4ConstraintSegment<T>>> lhs_constraints(lhs.mesh.faces.size());
    std::vector<std::vector<Boolean4ConstraintSegment<T>>> rhs_constraints(rhs.mesh.faces.size());

    for(const auto &candidate : BuildBoolean4TrianglePairBroadPhase(lhs, rhs)) {
        const auto lhs_tri = Boolean4FaceTriangle(lhs, candidate.lhs_face_id);
        const auto rhs_tri = Boolean4FaceTriangle(rhs, candidate.rhs_face_id);
        const auto classification = ClassifyBoolean4TriangleTriangle(lhs_tri, rhs_tri);
        if(classification.constraint_vertices.size() < 2UL) {
            continue;
        }

        const Boolean4ConstraintSegment<T> segment = {
            classification.constraint_vertices[0].approximate,
            classification.constraint_vertices[1].approximate };
        lhs_constraints.at(candidate.lhs_face_id).push_back(segment);
        rhs_constraints.at(candidate.rhs_face_id).push_back(segment);
    }

    return { BuildBoolean4OperandArrangement(lhs, lhs_constraints),
             BuildBoolean4OperandArrangement(rhs, rhs_constraints) };
}


template <class T>
vec3<T>
Boolean4FacetPoint(const Boolean4SplitTriangleResult<T> &split,
                   const Boolean4ArrangementFacet<T> &facet) {
    return (split.approximate_vertices.at(facet.vertex_ids[0])
          + split.approximate_vertices.at(facet.vertex_ids[1])
          + split.approximate_vertices.at(facet.vertex_ids[2])) / static_cast<T>(3);
}


template <class T>
vec3<T>
Boolean4FacetNormal(const Boolean4SplitTriangleResult<T> &split,
                    const Boolean4ArrangementFacet<T> &facet) {
    const auto &a = split.approximate_vertices.at(facet.vertex_ids[0]);
    const auto &b = split.approximate_vertices.at(facet.vertex_ids[1]);
    const auto &c = split.approximate_vertices.at(facet.vertex_ids[2]);
    return (b - a).Cross(c - a);
}


template <class T>
bool
Boolean4PointOnTriangle(const vec3<T> &p,
                        const std::array<vec3<T>, 3> &tri) {
    if(orient_sign(tri[0], tri[1], tri[2], p) != 0) {
        return false;
    }
    const int axis = Boolean4DominantProjectionAxis(tri);
    const auto projected = Boolean4ProjectTriangle(tri, axis);
    return point_in_triangle_or_on_boundary(Boolean4ProjectPoint(p, axis),
                                           projected[0], projected[1], projected[2]);
}


template <class T, class I>
Boolean4BoundaryFacing
Boolean4BoundaryFacingAgainstMesh(const vec3<T> &facet_normal,
                                  const vec3<T> &p,
                                  const Boolean4NormalizedInput<T, I> &opposite) {
    for(uint64_t face_id = 0; face_id < opposite.mesh.faces.size(); ++face_id) {
        const auto tri = Boolean4FaceTriangle(opposite, face_id);
        if(!Boolean4PointOnTriangle(p, tri)) {
            continue;
        }
        const auto opposite_normal = (tri[1] - tri[0]).Cross(tri[2] - tri[0]);
        return facet_normal.Dot(opposite_normal) >= static_cast<T>(0)
             ? Boolean4BoundaryFacing::SameFacing
             : Boolean4BoundaryFacing::OppositeFacing;
    }
    return Boolean4BoundaryFacing::None;
}


template <class T>
bool
Boolean4RayTriangleIntersectionT(const vec3<T> &origin,
                                 const vec3<T> &direction,
                                 const std::array<vec3<T>, 3> &tri,
                                 T &intersection_t,
                                 bool &ray_is_degenerate) {
    const auto edge1 = tri[1] - tri[0];
    const auto edge2 = tri[2] - tri[0];
    const auto h = direction.Cross(edge2);
    const T a = edge1.Dot(h);
    const T eps = std::numeric_limits<T>::epsilon() * static_cast<T>(1024);
    if(std::abs(a) <= eps) {
        return false;
    }

    const T f = static_cast<T>(1) / a;
    const auto s = origin - tri[0];
    const T u = f * s.Dot(h);
    if(u < -eps || u > static_cast<T>(1) + eps) {
        return false;
    }

    const auto q = s.Cross(edge1);
    const T v = f * direction.Dot(q);
    if(v < -eps || u + v > static_cast<T>(1) + eps) {
        return false;
    }

    const T t = f * edge2.Dot(q);
    if(t <= eps) {
        return false;
    }

    if(std::abs(u) <= eps || std::abs(v) <= eps || std::abs(static_cast<T>(1) - u - v) <= eps) {
        ray_is_degenerate = true;
    }

    intersection_t = t;
    return true;
}


template <class T, class I>
bool
Boolean4PointInsideClosedMesh(const vec3<T> &p,
                              const Boolean4NormalizedInput<T, I> &mesh) {
    const std::array<vec3<T>, 3> directions = {
        vec3<T>(static_cast<T>(1), static_cast<T>(0.375), static_cast<T>(0.125)),
        vec3<T>(static_cast<T>(0.25), static_cast<T>(1), static_cast<T>(0.5)),
        vec3<T>(static_cast<T>(0.5), static_cast<T>(0.125), static_cast<T>(1)) };

    for(const auto &direction : directions) {
        std::vector<T> hits;
        bool ray_is_degenerate = false;
        for(uint64_t face_id = 0; face_id < mesh.mesh.faces.size(); ++face_id) {
            T t = static_cast<T>(0);
            if(Boolean4RayTriangleIntersectionT(p, direction, Boolean4FaceTriangle(mesh, face_id), t, ray_is_degenerate)) {
                hits.push_back(t);
            }
        }
        if(ray_is_degenerate) {
            continue;
        }
        if(hits.empty()) {
            return false;
        }

        std::sort(hits.begin(), hits.end());
        std::vector<T> unique_hits;
        for(const auto t : hits) {
            const T tol = std::max(static_cast<T>(1), std::abs(t))
                        * std::numeric_limits<T>::epsilon() * static_cast<T>(4096);
            if(unique_hits.empty() || std::abs(t - unique_hits.back()) > tol) {
                unique_hits.push_back(t);
            }
        }
        return (unique_hits.size() % 2UL) == 1UL;
    }

    return false;
}


template <class T, class I>
Boolean4ClassifiedFacet<T>
ClassifyBoolean4ArrangementFacet(const Boolean4SplitTriangleResult<T> &split,
                                 uint64_t split_face_id,
                                 const Boolean4ArrangementFacet<T> &facet,
                                 const Boolean4NormalizedInput<T, I> &opposite) {
    Boolean4ClassifiedFacet<T> classified;
    classified.split_face_id = split_face_id;
    classified.facet_id = facet.stable_id;
    classified.source_face = facet.source_face;

    const auto p = Boolean4FacetPoint(split, facet);
    const auto n = Boolean4FacetNormal(split, facet);
    classified.boundary_facing = Boolean4BoundaryFacingAgainstMesh(n, p, opposite);
    if(classified.boundary_facing != Boolean4BoundaryFacing::None) {
        classified.location = Boolean4FacetLocation::OnBoundary;
        return classified;
    }

    classified.location = Boolean4PointInsideClosedMesh(p, opposite)
                        ? Boolean4FacetLocation::Inside
                        : Boolean4FacetLocation::Outside;
    return classified;
}


template <class T, class I>
Boolean4OperandFacetClassification<T>
ClassifyBoolean4OperandArrangementFacets(const Boolean4OperandArrangement<T> &arrangement,
                                         const Boolean4NormalizedInput<T, I> &opposite) {
    Boolean4OperandFacetClassification<T> classified;
    for(uint64_t split_face_id = 0; split_face_id < arrangement.split_faces.size(); ++split_face_id) {
        const auto &split = arrangement.split_faces[split_face_id];
        for(const auto &facet : split.facets) {
            classified.facets.push_back(ClassifyBoolean4ArrangementFacet(split, split_face_id, facet, opposite));
        }
    }
    std::sort(classified.facets.begin(), classified.facets.end(), [](const auto &lhs, const auto &rhs) {
        return std::tie(lhs.source_face, lhs.split_face_id, lhs.facet_id)
             < std::tie(rhs.source_face, rhs.split_face_id, rhs.facet_id);
    });
    return classified;
}


template <class T, class I>
Boolean4PairFacetClassification<T>
ClassifyBoolean4PairArrangementFacets(const Boolean4PairArrangement<T> &arrangement,
                                      const Boolean4NormalizedInput<T, I> &lhs,
                                      const Boolean4NormalizedInput<T, I> &rhs) {
    return { ClassifyBoolean4OperandArrangementFacets(arrangement.lhs, rhs),
             ClassifyBoolean4OperandArrangementFacets(arrangement.rhs, lhs) };
}


template <class T>
Boolean4FacetDecision<T>
MakeBoolean4FacetDecision(const Boolean4ClassifiedFacet<T> &facet,
                          Boolean4Operand operand,
                          bool emit,
                          bool reverse_orientation,
                          Boolean4FacetDecisionReason reason) {
    Boolean4FacetDecision<T> decision;
    decision.emit = emit;
    decision.reverse_orientation = reverse_orientation;
    decision.operand = operand;
    decision.split_face_id = facet.split_face_id;
    decision.facet_id = facet.facet_id;
    decision.source_face = facet.source_face;
    decision.reason = reason;
    return decision;
}


template <class T>
Boolean4FacetDecision<T>
SelectBoolean4LhsFacet(const Boolean4ClassifiedFacet<T> &facet,
                       MeshBooleanOperation4 op) {
    switch(op) {
        case MeshBooleanOperation4::Union:
            if(facet.location == Boolean4FacetLocation::Outside) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, true, false,
                                                 Boolean4FacetDecisionReason::LhsOutsideKept);
            }
            if(facet.location == Boolean4FacetLocation::OnBoundary
            && facet.boundary_facing == Boolean4BoundaryFacing::SameFacing) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, true, false,
                                                 Boolean4FacetDecisionReason::LhsSameBoundaryKeptAsRepresentative);
            }
            return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, false, false,
                                             facet.location == Boolean4FacetLocation::Inside
                                             ? Boolean4FacetDecisionReason::LhsInsideDiscarded
                                             : Boolean4FacetDecisionReason::LhsOppositeBoundaryDiscarded);

        case MeshBooleanOperation4::Intersection:
            if(facet.location == Boolean4FacetLocation::Inside) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, true, false,
                                                 Boolean4FacetDecisionReason::LhsInsideKept);
            }
            if(facet.location == Boolean4FacetLocation::OnBoundary
            && facet.boundary_facing == Boolean4BoundaryFacing::SameFacing) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, true, false,
                                                 Boolean4FacetDecisionReason::LhsSameBoundaryKeptAsRepresentative);
            }
            return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, false, false,
                                             facet.location == Boolean4FacetLocation::Outside
                                             ? Boolean4FacetDecisionReason::LhsSameBoundaryDiscarded
                                             : Boolean4FacetDecisionReason::LhsOppositeBoundaryDiscarded);

        case MeshBooleanOperation4::Subtraction:
            if(facet.location == Boolean4FacetLocation::Outside) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, true, false,
                                                 Boolean4FacetDecisionReason::LhsOutsideKept);
            }
            if(facet.location == Boolean4FacetLocation::OnBoundary
            && facet.boundary_facing == Boolean4BoundaryFacing::OppositeFacing) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, true, false,
                                                 Boolean4FacetDecisionReason::LhsOppositeBoundaryKeptForSubtractionContact);
            }
            return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, false, false,
                                             facet.location == Boolean4FacetLocation::Inside
                                             ? Boolean4FacetDecisionReason::LhsInsideDiscarded
                                             : Boolean4FacetDecisionReason::LhsSameBoundaryDiscarded);

        case MeshBooleanOperation4::Exclusion:
            if(facet.location == Boolean4FacetLocation::Outside) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, true, false,
                                                 Boolean4FacetDecisionReason::LhsOutsideKept);
            }
            if(facet.location == Boolean4FacetLocation::Inside) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, true, true,
                                                 Boolean4FacetDecisionReason::LhsInsideKeptReversedForExclusion);
            }
            return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, false, false,
                                             facet.boundary_facing == Boolean4BoundaryFacing::SameFacing
                                             ? Boolean4FacetDecisionReason::LhsSameBoundaryDiscarded
                                             : Boolean4FacetDecisionReason::LhsOppositeBoundaryDiscarded);
    }
    return MakeBoolean4FacetDecision(facet, Boolean4Operand::Lhs, false, false,
                                     Boolean4FacetDecisionReason::LhsInsideDiscarded);
}


template <class T>
Boolean4FacetDecision<T>
SelectBoolean4RhsFacet(const Boolean4ClassifiedFacet<T> &facet,
                       MeshBooleanOperation4 op) {
    switch(op) {
        case MeshBooleanOperation4::Union:
            if(facet.location == Boolean4FacetLocation::Outside) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Rhs, true, false,
                                                 Boolean4FacetDecisionReason::RhsOutsideKept);
            }
            return MakeBoolean4FacetDecision(facet, Boolean4Operand::Rhs, false, false,
                                             facet.location == Boolean4FacetLocation::Inside
                                             ? Boolean4FacetDecisionReason::RhsInsideDiscarded
                                             : (facet.boundary_facing == Boolean4BoundaryFacing::SameFacing
                                                ? Boolean4FacetDecisionReason::RhsSameBoundaryDiscardedAsDuplicate
                                                : Boolean4FacetDecisionReason::RhsOppositeBoundaryDiscarded));

        case MeshBooleanOperation4::Intersection:
            if(facet.location == Boolean4FacetLocation::Inside) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Rhs, true, false,
                                                 Boolean4FacetDecisionReason::RhsInsideKept);
            }
            return MakeBoolean4FacetDecision(facet, Boolean4Operand::Rhs, false, false,
                                             facet.location == Boolean4FacetLocation::Outside
                                             ? Boolean4FacetDecisionReason::RhsInsideDiscarded
                                             : (facet.boundary_facing == Boolean4BoundaryFacing::SameFacing
                                                ? Boolean4FacetDecisionReason::RhsSameBoundaryDiscardedAsDuplicate
                                                : Boolean4FacetDecisionReason::RhsOppositeBoundaryDiscarded));

        case MeshBooleanOperation4::Subtraction:
            if(facet.location == Boolean4FacetLocation::Inside) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Rhs, true, true,
                                                 Boolean4FacetDecisionReason::RhsInsideKeptReversed);
            }
            return MakeBoolean4FacetDecision(facet, Boolean4Operand::Rhs, false, false,
                                             facet.location == Boolean4FacetLocation::Outside
                                             ? Boolean4FacetDecisionReason::RhsInsideDiscarded
                                             : (facet.boundary_facing == Boolean4BoundaryFacing::SameFacing
                                                ? Boolean4FacetDecisionReason::RhsSameBoundaryDiscardedAsDuplicate
                                                : Boolean4FacetDecisionReason::RhsOppositeBoundaryDiscarded));

        case MeshBooleanOperation4::Exclusion:
            if(facet.location == Boolean4FacetLocation::Outside) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Rhs, true, false,
                                                 Boolean4FacetDecisionReason::RhsOutsideKept);
            }
            if(facet.location == Boolean4FacetLocation::Inside) {
                return MakeBoolean4FacetDecision(facet, Boolean4Operand::Rhs, true, true,
                                                 Boolean4FacetDecisionReason::RhsInsideKeptReversedForExclusion);
            }
            return MakeBoolean4FacetDecision(facet, Boolean4Operand::Rhs, false, false,
                                             facet.boundary_facing == Boolean4BoundaryFacing::SameFacing
                                             ? Boolean4FacetDecisionReason::RhsSameBoundaryDiscardedAsDuplicate
                                             : Boolean4FacetDecisionReason::RhsOppositeBoundaryDiscarded);
    }
    return MakeBoolean4FacetDecision(facet, Boolean4Operand::Rhs, false, false,
                                     Boolean4FacetDecisionReason::RhsInsideDiscarded);
}


template <class T>
Boolean4OperandFacetDecisions<T>
SelectBoolean4OperandFacets(const Boolean4OperandFacetClassification<T> &classification,
                            Boolean4Operand operand,
                            MeshBooleanOperation4 op) {
    Boolean4OperandFacetDecisions<T> decisions;
    decisions.facets.reserve(classification.facets.size());
    for(const auto &facet : classification.facets) {
        decisions.facets.push_back(operand == Boolean4Operand::Lhs
                                 ? SelectBoolean4LhsFacet(facet, op)
                                 : SelectBoolean4RhsFacet(facet, op));
    }
    std::sort(decisions.facets.begin(), decisions.facets.end());
    return decisions;
}


template <class T>
Boolean4PairFacetDecisions<T>
SelectBoolean4PairFacets(const Boolean4PairFacetClassification<T> &classification,
                         MeshBooleanOperation4 op) {
    return { SelectBoolean4OperandFacets(classification.lhs, Boolean4Operand::Lhs, op),
             SelectBoolean4OperandFacets(classification.rhs, Boolean4Operand::Rhs, op) };
}


template <class T>
bool
Boolean4SameSnapKey(const Boolean4SnapCoord &lhs,
                    const Boolean4SnapCoord &rhs) {
    return !(lhs < rhs) && !(rhs < lhs);
}


template <class T>
const Boolean4ArrangementFacet<T> &
Boolean4FindArrangementFacet(const Boolean4SplitTriangleResult<T> &split,
                             uint64_t facet_id) {
    const auto it = std::find_if(split.facets.begin(), split.facets.end(),
        [&](const auto &facet) { return facet.stable_id == facet_id; });
    if(it == split.facets.end()) {
        throw std::runtime_error("Boolean4 output assembly referenced a missing arrangement facet");
    }
    return *it;
}


template <class T>
void
Boolean4AppendOutputFacetCandidates(const Boolean4OperandArrangement<T> &arrangement,
                                    const Boolean4OperandFacetDecisions<T> &decisions,
                                    std::vector<Boolean4OutputFacetCandidate<T>> &candidates) {
    for(const auto &decision : decisions.facets) {
        if(!decision.emit) {
            continue;
        }
        if(decision.split_face_id >= arrangement.split_faces.size()) {
            throw std::runtime_error("Boolean4 output assembly referenced a missing split face");
        }

        const auto &split = arrangement.split_faces.at(decision.split_face_id);
        const auto &facet = Boolean4FindArrangementFacet(split, decision.facet_id);
        std::array<uint64_t, 3> ids = facet.vertex_ids;
        if(decision.reverse_orientation) {
            std::swap(ids[1], ids[2]);
        }

        Boolean4OutputFacetCandidate<T> candidate;
        candidate.operand = decision.operand;
        candidate.split_face_id = decision.split_face_id;
        candidate.facet_id = decision.facet_id;
        candidate.source_face = decision.source_face;
        for(size_t i = 0; i < 3UL; ++i) {
            candidate.vertex_keys[i] = split.vertices.at(ids[i]).key;
            candidate.approximate_vertices[i] = split.approximate_vertices.at(ids[i]);
        }
        candidates.push_back(candidate);
    }
}


template <class T>
std::vector<Boolean4OutputFacetCandidate<T>>
Boolean4CollectOutputFacetCandidates(const Boolean4PairArrangement<T> &arrangement,
                                     const Boolean4PairFacetDecisions<T> &decisions) {
    std::vector<Boolean4OutputFacetCandidate<T>> candidates;
    Boolean4AppendOutputFacetCandidates(arrangement.lhs, decisions.lhs, candidates);
    Boolean4AppendOutputFacetCandidates(arrangement.rhs, decisions.rhs, candidates);
    std::sort(candidates.begin(), candidates.end());
    return candidates;
}


template <class T>
bool
Boolean4OutputFacetHasRepeatedVertices(const Boolean4OutputFacetCandidate<T> &facet) {
    return Boolean4SameSnapKey<T>(facet.vertex_keys[0], facet.vertex_keys[1])
        || Boolean4SameSnapKey<T>(facet.vertex_keys[1], facet.vertex_keys[2])
        || Boolean4SameSnapKey<T>(facet.vertex_keys[2], facet.vertex_keys[0]);
}


template <class T>
bool
Boolean4OutputFacetHasZeroArea(const Boolean4OutputFacetCandidate<T> &facet) {
    const auto normal = (facet.approximate_vertices[1] - facet.approximate_vertices[0])
                      .Cross(facet.approximate_vertices[2] - facet.approximate_vertices[0]);
    return normal.sq_length() == static_cast<T>(0);
}


template <class T>
std::vector<Boolean4OutputVertexCandidate<T>>
Boolean4CollectOutputVertices(const std::vector<Boolean4OutputFacetCandidate<T>> &facets) {
    std::vector<Boolean4OutputVertexCandidate<T>> vertices;
    vertices.reserve(facets.size() * 3UL);
    for(const auto &facet : facets) {
        for(size_t i = 0; i < 3UL; ++i) {
            vertices.push_back({ facet.vertex_keys[i], facet.approximate_vertices[i] });
        }
    }

    std::sort(vertices.begin(), vertices.end());
    std::vector<Boolean4OutputVertexCandidate<T>> unique_vertices;
    for(const auto &vertex : vertices) {
        if(unique_vertices.empty() || !Boolean4SameSnapKey<T>(unique_vertices.back().key, vertex.key)) {
            unique_vertices.push_back(vertex);
        }
    }
    return unique_vertices;
}


template <class T>
uint64_t
Boolean4OutputVertexId(const std::vector<Boolean4OutputVertexCandidate<T>> &vertices,
                       const Boolean4SnapCoord &key) {
    const auto it = std::lower_bound(vertices.begin(), vertices.end(), key,
        [](const auto &vertex, const auto &needle) { return vertex.key < needle; });
    if(it == vertices.end() || !Boolean4SameSnapKey<T>(it->key, key)) {
        throw std::runtime_error("Boolean4 output assembly could not resolve a topology vertex");
    }
    return static_cast<uint64_t>(std::distance(vertices.begin(), it));
}


template <class I>
I
Boolean4CheckedIndex(uint64_t value,
                     const char *what) {
    if(value > static_cast<uint64_t>(std::numeric_limits<I>::max())) {
        throw std::runtime_error(std::string("Boolean4 output has too many ") + what + " for the requested index type");
    }
    return static_cast<I>(value);
}


template <class I>
void
Boolean4RequireCountFitsIndex(uint64_t count,
                              const char *what) {
    const uint64_t max_index = static_cast<uint64_t>(std::numeric_limits<I>::max());
    if(count != 0UL && count - 1UL > max_index) {
        throw std::runtime_error(std::string("Boolean4 output has too many ") + what + " for the requested index type");
    }
}


inline std::array<uint64_t, 3>
Boolean4CanonicalFaceVertexIds(std::array<uint64_t, 3> ids) {
    std::sort(ids.begin(), ids.end());
    return ids;
}


inline void
Boolean4ValidateOutputHalfedges(std::vector<Boolean4OutputHalfedgeCandidate> halfedges) {
    std::sort(halfedges.begin(), halfedges.end());
    for(size_t i = 0; i < halfedges.size();) {
        size_t j = i + 1UL;
        while(j < halfedges.size()
           && halfedges[j].undirected_min == halfedges[i].undirected_min
           && halfedges[j].undirected_max == halfedges[i].undirected_max) {
            ++j;
        }

        if(j - i != 2UL) {
            throw std::runtime_error("Boolean4 output assembly produced a non-manifold edge between vertices "
                                   + std::to_string(halfedges[i].undirected_min)
                                   + " and "
                                   + std::to_string(halfedges[i].undirected_max)
                                   + " with "
                                   + std::to_string(j - i)
                                   + " incident faces");
        }
        if(halfedges[i].origin != halfedges[i + 1UL].target
        || halfedges[i].target != halfedges[i + 1UL].origin) {
            throw std::runtime_error("Boolean4 output assembly produced paired halfedges with the same orientation");
        }
        i = j;
    }
}


template <class T, class I>
fv_surface_mesh<T, I>
AssembleBoolean4OutputHalfedgeMesh(const Boolean4PairArrangement<T> &arrangement,
                                   const Boolean4PairFacetDecisions<T> &decisions) {
    auto facets = Boolean4CollectOutputFacetCandidates(arrangement, decisions);
    facets.erase(std::remove_if(facets.begin(), facets.end(), [](const auto &facet) {
        return Boolean4OutputFacetHasRepeatedVertices(facet)
            || Boolean4OutputFacetHasZeroArea(facet);
    }), facets.end());

    fv_surface_mesh<T, I> mesh;
    if(facets.empty()) {
        return mesh;
    }

    const auto vertices = Boolean4CollectOutputVertices(facets);
    Boolean4RequireCountFitsIndex<I>(static_cast<uint64_t>(vertices.size()), "vertices");
    mesh.vertices.reserve(vertices.size());
    for(uint64_t vertex_id = 0; vertex_id < vertices.size(); ++vertex_id) {
        (void)Boolean4CheckedIndex<I>(vertex_id, "vertices");
        mesh.vertices.push_back(vertices[vertex_id].approximate);
    }

    std::vector<std::array<uint64_t, 3>> face_vertex_ids;
    face_vertex_ids.reserve(facets.size());
    for(const auto &facet : facets) {
        const std::array<uint64_t, 3> ids = {
            Boolean4OutputVertexId(vertices, facet.vertex_keys[0]),
            Boolean4OutputVertexId(vertices, facet.vertex_keys[1]),
            Boolean4OutputVertexId(vertices, facet.vertex_keys[2]) };
        if(ids[0] == ids[1] || ids[1] == ids[2] || ids[2] == ids[0]) {
            continue;
        }
        face_vertex_ids.push_back(ids);
    }

    std::sort(face_vertex_ids.begin(), face_vertex_ids.end(), [](const auto &lhs, const auto &rhs) {
        const auto lhs_key = Boolean4CanonicalFaceVertexIds(lhs);
        const auto rhs_key = Boolean4CanonicalFaceVertexIds(rhs);
        return std::tie(lhs_key, lhs) < std::tie(rhs_key, rhs);
    });
    face_vertex_ids.erase(std::unique(face_vertex_ids.begin(), face_vertex_ids.end(), [](const auto &lhs, const auto &rhs) {
        return Boolean4CanonicalFaceVertexIds(lhs) == Boolean4CanonicalFaceVertexIds(rhs);
    }), face_vertex_ids.end());

    Boolean4RequireCountFitsIndex<I>(static_cast<uint64_t>(face_vertex_ids.size()), "faces");

    std::vector<Boolean4OutputHalfedgeCandidate> halfedges;
    halfedges.reserve(face_vertex_ids.size() * 3UL);
    for(uint64_t face_id = 0; face_id < face_vertex_ids.size(); ++face_id) {
        (void)Boolean4CheckedIndex<I>(face_id, "faces");
        const auto &ids = face_vertex_ids[face_id];
        mesh.faces.push_back({ Boolean4CheckedIndex<I>(ids[0], "vertices"),
                               Boolean4CheckedIndex<I>(ids[1], "vertices"),
                               Boolean4CheckedIndex<I>(ids[2], "vertices") });
        for(uint64_t edge_id = 0; edge_id < 3UL; ++edge_id) {
            const auto origin = ids[edge_id];
            const auto target = ids[(edge_id + 1UL) % 3UL];
            Boolean4OutputHalfedgeCandidate halfedge;
            halfedge.stable_id = static_cast<uint64_t>(halfedges.size());
            halfedge.face_id = face_id;
            halfedge.origin = origin;
            halfedge.target = target;
            halfedge.undirected_min = std::min(origin, target);
            halfedge.undirected_max = std::max(origin, target);
            halfedge.next_halfedge_id = face_id * 3UL + ((edge_id + 1UL) % 3UL);
            halfedges.push_back(halfedge);
        }
    }

    Boolean4ValidateOutputHalfedges(std::move(halfedges));
    mesh.recreate_involved_face_index();
    return mesh;
}


bool
IsValidOperation4(MeshBooleanOperation4 op) {
    switch(op) {
        case MeshBooleanOperation4::Union:
        case MeshBooleanOperation4::Intersection:
        case MeshBooleanOperation4::Exclusion:
        case MeshBooleanOperation4::Subtraction:
            return true;
    }
    return false;
}


const char *
Boolean4OperationName(MeshBooleanOperation4 op) {
    switch(op) {
        case MeshBooleanOperation4::Union:
            return "union";
        case MeshBooleanOperation4::Intersection:
            return "intersection";
        case MeshBooleanOperation4::Exclusion:
            return "exclusion";
        case MeshBooleanOperation4::Subtraction:
            return "subtraction";
    }
    return "invalid";
}


[[maybe_unused]] const char *
Boolean4FacetDecisionReasonName(Boolean4FacetDecisionReason reason) {
    switch(reason) {
        case Boolean4FacetDecisionReason::LhsOutsideKept:
            return "lhs outside kept";
        case Boolean4FacetDecisionReason::LhsInsideKept:
            return "lhs inside kept";
        case Boolean4FacetDecisionReason::LhsInsideDiscarded:
            return "lhs inside discarded";
        case Boolean4FacetDecisionReason::LhsSameBoundaryKeptAsRepresentative:
            return "lhs same-boundary kept as representative";
        case Boolean4FacetDecisionReason::LhsSameBoundaryDiscarded:
            return "lhs same-boundary discarded";
        case Boolean4FacetDecisionReason::LhsOppositeBoundaryKeptForSubtractionContact:
            return "lhs opposite-boundary kept for subtraction contact";
        case Boolean4FacetDecisionReason::LhsOppositeBoundaryDiscarded:
            return "lhs opposite-boundary discarded";
        case Boolean4FacetDecisionReason::LhsInsideKeptReversedForExclusion:
            return "lhs inside kept reversed for exclusion";
        case Boolean4FacetDecisionReason::RhsOutsideKept:
            return "rhs outside kept";
        case Boolean4FacetDecisionReason::RhsInsideKept:
            return "rhs inside kept";
        case Boolean4FacetDecisionReason::RhsInsideDiscarded:
            return "rhs inside discarded";
        case Boolean4FacetDecisionReason::RhsInsideKeptReversed:
            return "rhs inside kept reversed";
        case Boolean4FacetDecisionReason::RhsSameBoundaryDiscardedAsDuplicate:
            return "rhs same-boundary discarded as duplicate";
        case Boolean4FacetDecisionReason::RhsOppositeBoundaryDiscarded:
            return "rhs opposite-boundary discarded";
        case Boolean4FacetDecisionReason::RhsInsideKeptReversedForExclusion:
            return "rhs inside kept reversed for exclusion";
    }
    return "invalid facet decision reason";
}


template <class T>
std::string
Boolean4DebugDump(const Boolean4PairArrangement<T> &arrangement,
                  const Boolean4PairFacetClassification<T> &classification,
                  const Boolean4PairFacetDecisions<T> &decisions,
                  MeshBooleanOperation4 op) {
    std::ostringstream os;
    os << std::setprecision(17);
    os << "Boolean4 debug dump\n";
    os << "operation " << Boolean4OperationName(op) << '\n';

    const auto dump_arrangement = [&](const char *name, const Boolean4OperandArrangement<T> &operand_arrangement) {
        os << "arrangement " << name << " split_faces " << operand_arrangement.split_faces.size() << '\n';
        for(uint64_t split_face_id = 0; split_face_id < operand_arrangement.split_faces.size(); ++split_face_id) {
            const auto &split = operand_arrangement.split_faces[split_face_id];
            os << "split_face " << split_face_id
               << " vertices " << split.vertices.size()
               << " facets " << split.facets.size() << '\n';
            for(uint64_t vertex_id = 0; vertex_id < split.vertices.size(); ++vertex_id) {
                const auto &v = split.vertices[vertex_id];
                os << "vertex " << vertex_id
                   << " key " << v.key.binary_exponent << ' ' << v.key.x << ' ' << v.key.y << ' ' << v.key.z << '\n';
            }
            for(const auto &facet : split.facets) {
                os << "facet " << facet.stable_id
                   << " source_operand " << Boolean4OperandName(facet.source_face.operand)
                   << " source_face " << facet.source_face.face_id
                   << " vertices " << facet.vertex_ids[0] << ' ' << facet.vertex_ids[1] << ' ' << facet.vertex_ids[2] << '\n';
            }
        }
    };

    const auto dump_classification = [&](const char *name, const Boolean4OperandFacetClassification<T> &operand_classification) {
        os << "classification " << name << " facets " << operand_classification.facets.size() << '\n';
        for(const auto &facet : operand_classification.facets) {
            os << "classified split_face " << facet.split_face_id
               << " facet " << facet.facet_id
               << " source_face " << facet.source_face.face_id
               << " location " << Boolean4FacetLocationName(facet.location)
               << " boundary_facing " << Boolean4BoundaryFacingName(facet.boundary_facing) << '\n';
        }
    };

    const auto dump_decisions = [&](const char *name, const Boolean4OperandFacetDecisions<T> &operand_decisions) {
        os << "decisions " << name << " facets " << operand_decisions.facets.size() << '\n';
        for(const auto &decision : operand_decisions.facets) {
            os << "decision split_face " << decision.split_face_id
               << " facet " << decision.facet_id
               << " source_face " << decision.source_face.face_id
               << " emit " << decision.emit
               << " reverse " << decision.reverse_orientation
               << " reason \"" << Boolean4FacetDecisionReasonName(decision.reason) << "\"\n";
        }
    };

    dump_arrangement("lhs", arrangement.lhs);
    dump_arrangement("rhs", arrangement.rhs);
    dump_classification("lhs", classification.lhs);
    dump_classification("rhs", classification.rhs);
    dump_decisions("lhs", decisions.lhs);
    dump_decisions("rhs", decisions.rhs);
    return os.str();
}


template <class T>
void
Boolean4MaybeWriteDebugDump(const Boolean4PairArrangement<T> &arrangement,
                            const Boolean4PairFacetClassification<T> &classification,
                            const Boolean4PairFacetDecisions<T> &decisions,
                            MeshBooleanOperation4 op) {
#ifdef YGOR_MESHES_BOOLEAN4_ENABLE_DEBUG_DUMP
    const char *path = std::getenv("YGOR_MESHES_BOOLEAN4_DEBUG_DUMP");
    if(path == nullptr || *path == '\0') {
        return;
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if(!out) {
        throw std::runtime_error("Boolean4 could not open debug dump path");
    }
    out << Boolean4DebugDump(arrangement, classification, decisions, op);
#else
    (void)arrangement;
    (void)classification;
    (void)decisions;
    (void)op;
#endif
}


template <class T, class I>
bool
Boolean4HasNoDuplicateFaces(const fv_surface_mesh<T, I> &mesh) {
    std::vector<std::array<I, 3>> faces;
    faces.reserve(mesh.faces.size());
    for(const auto &face : mesh.faces) {
        if(face.size() != 3UL) {
            continue;
        }
        std::array<I, 3> ids = { { face[0], face[1], face[2] } };
        std::sort(ids.begin(), ids.end());
        faces.push_back(ids);
    }
    std::sort(faces.begin(), faces.end());
    return std::adjacent_find(faces.begin(), faces.end()) == faces.end();
}


template <class T, class I>
void
VerifyBoolean4OutputPostconditions(const fv_surface_mesh<T, I> &mesh,
                                   MeshBooleanOperation4 op) {
    if(mesh.faces.empty()) {
        return;
    }

    const std::string prefix = std::string("BooleanMeshOp4 ")
                           + Boolean4OperationName(op)
                           + " output failed postcondition: ";
    if(!HasOnlyFiniteVertices(mesh)) {
        throw std::runtime_error(prefix + "non-finite vertices");
    }
    if(!IsTriangularMesh(mesh)) {
        throw std::runtime_error(prefix + "non-triangular faces");
    }
    if(!HasValidFaceIndices(mesh)) {
        throw std::runtime_error(prefix + "invalid face indices");
    }
    if(!HasNoDegenerateFaces(mesh)) {
        throw std::runtime_error(prefix + "degenerate faces");
    }
    if(!Boolean4HasNoDuplicateFaces(mesh)) {
        throw std::runtime_error(prefix + "duplicate faces");
    }
    if(!IsClosedManifold(mesh)) {
        throw std::runtime_error(prefix + "not a closed manifold");
    }
    if(!HasConsistentOrientation(mesh)) {
        throw std::runtime_error(prefix + "inconsistent orientation");
    }
}


template <class T, class I>
fv_surface_mesh<T, I>
AssembleNormalizedIdentityOperand4(const Boolean4NormalizedInput<T, I> &input,
                                    MeshBooleanOperation4 op) {
    Boolean4PairArrangement<T> arrangement;
    arrangement.lhs = BuildBoolean4OperandArrangement(input,
        std::vector<std::vector<Boolean4ConstraintSegment<T>>>(input.mesh.faces.size()));

    Boolean4PairFacetDecisions<T> decisions;
    decisions.lhs.facets.reserve(input.mesh.faces.size());
    for(uint64_t split_face_id = 0; split_face_id < arrangement.lhs.split_faces.size(); ++split_face_id) {
        const auto &split = arrangement.lhs.split_faces[split_face_id];
        for(const auto &facet : split.facets) {
            Boolean4FacetDecision<T> decision;
            decision.emit = true;
            decision.operand = Boolean4Operand::Lhs;
            decision.split_face_id = split_face_id;
            decision.facet_id = facet.stable_id;
            decision.source_face = facet.source_face;
            decision.reason = Boolean4FacetDecisionReason::LhsOutsideKept;
            decisions.lhs.facets.push_back(decision);
        }
    }
    std::sort(decisions.lhs.facets.begin(), decisions.lhs.facets.end());

    auto result = AssembleBoolean4OutputHalfedgeMesh<T, I>(arrangement, decisions);
    VerifyBoolean4OutputPostconditions(result, op);
    return result;
}


template <class T, class I>
void
Boolean4AppendMeshWithoutWelding(fv_surface_mesh<T, I> &dst,
                                 const fv_surface_mesh<T, I> &src) {
    const auto vertex_offset = Boolean4CheckedIndex<I>(static_cast<uint64_t>(dst.vertices.size()), "vertices");
    dst.vertices.insert(dst.vertices.end(), src.vertices.begin(), src.vertices.end());
    for(const auto &face : src.faces) {
        std::vector<I> appended_face;
        appended_face.reserve(face.size());
        for(const auto idx : face) {
            appended_face.push_back(Boolean4CheckedIndex<I>(static_cast<uint64_t>(vertex_offset) + static_cast<uint64_t>(idx), "vertices"));
        }
        dst.faces.push_back(std::move(appended_face));
    }
}


template <class T, class I>
fv_surface_mesh<T, I>
AssembleBoolean4ClosedComponentFallback(const Boolean4NormalizedInput<T, I> &lhs,
                                        const Boolean4NormalizedInput<T, I> &rhs,
                                        MeshBooleanOperation4 op) {
    switch(op) {
        case MeshBooleanOperation4::Intersection:
            return fv_surface_mesh<T, I>();
        case MeshBooleanOperation4::Subtraction:
            return AssembleNormalizedIdentityOperand4(lhs, op);
        case MeshBooleanOperation4::Union:
        case MeshBooleanOperation4::Exclusion:
            break;
    }

    fv_surface_mesh<T, I> result;
    Boolean4AppendMeshWithoutWelding(result, AssembleNormalizedIdentityOperand4(lhs, op));
    Boolean4AppendMeshWithoutWelding(result, AssembleNormalizedIdentityOperand4(rhs, op));
    result.recreate_involved_face_index();
    VerifyBoolean4OutputPostconditions(result, op);
    return result;
}


template <class T, class I>
fv_surface_mesh<T, I>
RunBoolean4ArrangementPipeline(const Boolean4NormalizedInput<T, I> &lhs,
                               const Boolean4NormalizedInput<T, I> &rhs,
                               MeshBooleanOperation4 op) {
    const auto arrangement = BuildBoolean4PairArrangement(lhs, rhs);
    const auto classification = ClassifyBoolean4PairArrangementFacets(arrangement, lhs, rhs);
    const auto decisions = SelectBoolean4PairFacets(classification, op);
    Boolean4MaybeWriteDebugDump(arrangement, classification, decisions, op);
    auto result = AssembleBoolean4OutputHalfedgeMesh<T, I>(arrangement, decisions);
    VerifyBoolean4OutputPostconditions(result, op);
    return result;
}

} // namespace


template <class T, class I>
fv_surface_mesh<T, I>
BooleanMeshOp4(const fv_surface_mesh<T, I> &lhs,
               const fv_surface_mesh<T, I> &rhs,
               MeshBooleanOperation4 op) {
    if(!IsValidOperation4(op)) {
        throw std::invalid_argument("BooleanMeshOp4 received an invalid MeshBooleanOperation4 value");
    }

    const auto lhs_input = NormalizeAndValidateInput4(lhs, "lhs", Boolean4Operand::Lhs);
    const auto rhs_input = NormalizeAndValidateInput4(rhs, "rhs", Boolean4Operand::Rhs);

    if(lhs_input.mesh.faces.empty() || lhs_input.mesh.vertices.empty()) {
        switch(op) {
            case MeshBooleanOperation4::Union:
            case MeshBooleanOperation4::Exclusion:
                return (rhs_input.mesh.faces.empty() || rhs_input.mesh.vertices.empty())
                      ? fv_surface_mesh<T, I>()
                     : AssembleNormalizedIdentityOperand4(rhs_input, op);
            case MeshBooleanOperation4::Intersection:
            case MeshBooleanOperation4::Subtraction:
                return fv_surface_mesh<T, I>();
        }
    }
    if(rhs_input.mesh.faces.empty() || rhs_input.mesh.vertices.empty()) {
        switch(op) {
            case MeshBooleanOperation4::Union:
            case MeshBooleanOperation4::Subtraction:
            case MeshBooleanOperation4::Exclusion:
                return AssembleNormalizedIdentityOperand4(lhs_input, op);
            case MeshBooleanOperation4::Intersection:
                return fv_surface_mesh<T, I>();
        }
    }

    Boolean4Cuboid<T> lhs_cuboid;
    Boolean4Cuboid<T> rhs_cuboid;
    const bool lhs_is_cuboid = Boolean4DetectAxisAlignedCuboid(lhs_input, lhs_cuboid);
    const bool rhs_is_cuboid = Boolean4DetectAxisAlignedCuboid(rhs_input, rhs_cuboid);
    if(lhs_is_cuboid && rhs_is_cuboid) {
        return RunBoolean4CuboidCellArrangement<T, I>(lhs_cuboid, rhs_cuboid, op);
    }
    if(lhs_is_cuboid != rhs_is_cuboid) {
        return AssembleBoolean4ClosedComponentFallback(lhs_input, rhs_input, op);
    }

    return RunBoolean4ArrangementPipeline(lhs_input, rhs_input, op);
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
