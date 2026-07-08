#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <YgorMath.h>
#include <YgorMeshesBoolean4.h>
#include <YgorMeshesVerification.h>

#define YGOR_MESHES_BOOLEAN4_DISABLE_ALL_SPECIALIZATIONS
#include "../src/YgorMeshesBoolean4.cc"

namespace {

using T = double;
using I = uint64_t;
using Mesh = fv_surface_mesh<T, I>;

constexpr T area_tol = 1.0E-9;
constexpr T volume_tol = 1.0E-9;

Mesh MakeCube(T xmin, T ymin, T zmin, T xmax, T ymax, T zmax) {
    Mesh mesh;
    mesh.vertices = {
        vec3<T>(xmin, ymin, zmin), vec3<T>(xmax, ymin, zmin),
        vec3<T>(xmax, ymax, zmin), vec3<T>(xmin, ymax, zmin),
        vec3<T>(xmin, ymin, zmax), vec3<T>(xmax, ymin, zmax),
        vec3<T>(xmax, ymax, zmax), vec3<T>(xmin, ymax, zmax),
    };
    mesh.faces = {
        {0, 2, 1}, {0, 3, 2},
        {4, 5, 6}, {4, 6, 7},
        {0, 1, 5}, {0, 5, 4},
        {3, 6, 2}, {3, 7, 6},
        {0, 4, 7}, {0, 7, 3},
        {1, 2, 6}, {1, 6, 5},
    };
    mesh.recreate_involved_face_index();
    return mesh;
}

Mesh MakeTetrahedron() {
    Mesh mesh;
    mesh.vertices = {
        vec3<T>(0.20, 0.20, 0.20),
        vec3<T>(1.20, 0.35, 0.30),
        vec3<T>(0.40, 1.10, 0.55),
        vec3<T>(0.60, 0.45, 1.35),
    };

    const std::vector<std::vector<I>> faces = {
        {0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3},
    };
    const vec3<T> centroid = (mesh.vertices[0] + mesh.vertices[1]
                           + mesh.vertices[2] + mesh.vertices[3]) * 0.25;
    for(auto face : faces) {
        const auto &a = mesh.vertices[face[0]];
        const auto &b = mesh.vertices[face[1]];
        const auto &c = mesh.vertices[face[2]];
        const vec3<T> face_center = (a + b + c) * (1.0 / 3.0);
        const vec3<T> normal = (b - a).Cross(c - a);
        if(normal.Dot(face_center - centroid) < 0.0) {
            std::swap(face[1], face[2]);
        }
        mesh.faces.emplace_back(std::move(face));
    }
    mesh.recreate_involved_face_index();
    return mesh;
}

T SignedVolume(const Mesh &mesh) {
    T volume = 0.0;
    for(const auto &face : mesh.faces) {
        if(face.size() != 3) {
            throw std::runtime_error("non-triangular face in SignedVolume");
        }
        const auto &a = mesh.vertices.at(face[0]);
        const auto &b = mesh.vertices.at(face[1]);
        const auto &c = mesh.vertices.at(face[2]);
        volume += a.Dot(b.Cross(c)) / 6.0;
    }
    return volume;
}

T Volume(const Mesh &mesh) {
    return std::abs(SignedVolume(mesh));
}

std::string MeshBytes(const Mesh &mesh) {
    std::ostringstream os;
    os << std::setprecision(17);
    os << "vertices " << mesh.vertices.size() << '\n';
    for(const auto &v : mesh.vertices) {
        os << v.x << ' ' << v.y << ' ' << v.z << '\n';
    }
    os << "faces " << mesh.faces.size() << '\n';
    for(const auto &face : mesh.faces) {
        os << face.size();
        for(const auto idx : face) {
            os << ' ' << idx;
        }
        os << '\n';
    }
    return os.str();
}

std::string CanonicalMeshBytes(const Mesh &mesh) {
    std::vector<std::pair<vec3<T>, I>> vertices;
    vertices.reserve(mesh.vertices.size());
    for(I i = 0; i < static_cast<I>(mesh.vertices.size()); ++i) {
        vertices.push_back({ mesh.vertices[i], i });
    }
    std::sort(vertices.begin(), vertices.end(), [](const auto &lhs, const auto &rhs) {
        return std::tie(lhs.first.x, lhs.first.y, lhs.first.z, lhs.second)
             < std::tie(rhs.first.x, rhs.first.y, rhs.first.z, rhs.second);
    });

    std::vector<I> remap(mesh.vertices.size(), 0);
    for(I i = 0; i < static_cast<I>(vertices.size()); ++i) {
        remap.at(vertices[i].second) = i;
    }

    std::vector<std::vector<I>> faces;
    faces.reserve(mesh.faces.size());
    for(const auto &face : mesh.faces) {
        std::vector<I> remapped;
        remapped.reserve(face.size());
        for(const auto idx : face) {
            remapped.push_back(remap.at(idx));
        }
        faces.push_back(std::move(remapped));
    }
    std::sort(faces.begin(), faces.end());

    std::ostringstream os;
    os << std::setprecision(17);
    os << "vertices " << vertices.size() << '\n';
    for(const auto &v : vertices) {
        os << v.first.x << ' ' << v.first.y << ' ' << v.first.z << '\n';
    }
    os << "faces " << faces.size() << '\n';
    for(const auto &face : faces) {
        os << face.size();
        for(const auto idx : face) {
            os << ' ' << idx;
        }
        os << '\n';
    }
    return os.str();
}

bool IsEmpty(const Mesh &mesh) {
    return mesh.faces.empty();
}

Mesh MakeVertexOnlyEmptyMesh() {
    Mesh mesh;
    mesh.vertices = { vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0) };
    mesh.recreate_involved_face_index();
    return mesh;
}

Mesh MakeCubeWithUnusedVertex() {
    Mesh mesh = MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    mesh.vertices.push_back(vec3<T>(42.0, 42.0, 42.0));
    mesh.recreate_involved_face_index();
    return mesh;
}

Mesh TranslateMesh(Mesh mesh, const vec3<T> &delta) {
    for(auto &vertex : mesh.vertices) {
        vertex = vertex + delta;
    }
    mesh.recreate_involved_face_index();
    return mesh;
}

Mesh ScaleMesh(Mesh mesh, T scale) {
    for(auto &vertex : mesh.vertices) {
        vertex = vertex * scale;
    }
    mesh.recreate_involved_face_index();
    return mesh;
}

Mesh Rotate90ZMesh(Mesh mesh) {
    for(auto &vertex : mesh.vertices) {
        vertex = vec3<T>(-vertex.y, vertex.x, vertex.z);
    }
    mesh.recreate_involved_face_index();
    return mesh;
}

struct FixedSeedGenerator {
    uint32_t state = 0;

    explicit FixedSeedGenerator(uint32_t seed) : state(seed) {}

    uint32_t Next() {
        state = state * 1664525U + 1013904223U;
        return state;
    }

    T Unit() {
        return static_cast<T>(Next() & 0x00FFFFFFU) / static_cast<T>(0x01000000U);
    }
};

Mesh MakeSeededConvexCube(uint32_t seed) {
    FixedSeedGenerator gen(seed);
    const T x = static_cast<T>(seed % 11U) * 4.0 + gen.Unit() * 0.25;
    const T y = static_cast<T>((seed / 11U) % 11U) * 4.0 + gen.Unit() * 0.25;
    const T z = static_cast<T>((seed / 121U) % 11U) * 4.0 + gen.Unit() * 0.25;
    const T sx = 0.75 + gen.Unit() * 0.5;
    const T sy = 0.75 + gen.Unit() * 0.5;
    const T sz = 0.75 + gen.Unit() * 0.5;
    return MakeCube(x, y, z, x + sx, y + sy, z + sz);
}

void Require(bool condition, const std::string &message) {
    if(!condition) {
        throw std::runtime_error(message);
    }
}

void RequireNear(T actual, T expected, T tol, const std::string &message) {
    if(std::abs(actual - expected) > tol) {
        std::ostringstream os;
        os << message << ": expected " << std::setprecision(17) << expected
           << ", got " << actual;
        throw std::runtime_error(os.str());
    }
}

void RequireSameMeshBytes(const Mesh &lhs,
                          const Mesh &rhs,
                          const std::string &label) {
    Require(MeshBytes(lhs) == MeshBytes(rhs), label + ": mesh byte streams differ");
}

void RequireSameCanonicalMeshBytes(const Mesh &lhs,
                                   const Mesh &rhs,
                                   const std::string &label) {
    Require(CanonicalMeshBytes(lhs) == CanonicalMeshBytes(rhs), label + ": canonical mesh byte streams differ");
}

void RequirePostconditions(const Mesh &mesh, const std::string &label) {
    if(IsEmpty(mesh)) {
        return;
    }
    Require(HasOnlyFiniteVertices(mesh), label + ": non-finite vertices");
    Require(IsTriangularMesh(mesh), label + ": non-triangular output");
    Require(HasValidFaceIndices(mesh), label + ": invalid face indices");
    Require(HasNoDegenerateFaces(mesh), label + ": degenerate faces");
    Require(IsClosedManifold(mesh), label + ": not a closed manifold");
    Require(HasConsistentOrientation(mesh), label + ": inconsistent orientation");
}

template <class TT, class II>
void RequireTypedPostconditions(const fv_surface_mesh<TT, II> &mesh,
                                const std::string &label) {
    if(mesh.faces.empty()) {
        return;
    }
    Require(HasOnlyFiniteVertices(mesh), label + ": non-finite vertices");
    Require(IsTriangularMesh(mesh), label + ": non-triangular output");
    Require(HasValidFaceIndices(mesh), label + ": invalid face indices");
    Require(HasNoDegenerateFaces(mesh), label + ": degenerate faces");
    Require(IsClosedManifold(mesh), label + ": not a closed manifold");
    Require(HasConsistentOrientation(mesh), label + ": inconsistent orientation");
}

template <class TT, class II>
fv_surface_mesh<TT, II> MakeTypedCube(TT xmin, TT ymin, TT zmin,
                                      TT xmax, TT ymax, TT zmax) {
    fv_surface_mesh<TT, II> mesh;
    mesh.vertices = {
        vec3<TT>(xmin, ymin, zmin), vec3<TT>(xmax, ymin, zmin),
        vec3<TT>(xmax, ymax, zmin), vec3<TT>(xmin, ymax, zmin),
        vec3<TT>(xmin, ymin, zmax), vec3<TT>(xmax, ymin, zmax),
        vec3<TT>(xmax, ymax, zmax), vec3<TT>(xmin, ymax, zmax),
    };
    mesh.faces = {
        {0, 2, 1}, {0, 3, 2},
        {4, 5, 6}, {4, 6, 7},
        {0, 1, 5}, {0, 5, 4},
        {3, 6, 2}, {3, 7, 6},
        {0, 4, 7}, {0, 7, 3},
        {1, 2, 6}, {1, 6, 5},
    };
    mesh.recreate_involved_face_index();
    return mesh;
}

template <class TT, class II>
std::string TypedMeshBytes(const fv_surface_mesh<TT, II> &mesh) {
    std::ostringstream os;
    os << std::setprecision(std::numeric_limits<TT>::max_digits10);
    os << "vertices " << mesh.vertices.size() << '\n';
    for(const auto &v : mesh.vertices) {
        os << v.x << ' ' << v.y << ' ' << v.z << '\n';
    }
    os << "faces " << mesh.faces.size() << '\n';
    for(const auto &face : mesh.faces) {
        os << face.size();
        for(const auto idx : face) {
            os << ' ' << static_cast<uint64_t>(idx);
        }
        os << '\n';
    }
    return os.str();
}

template <class TT, class II>
void RequireTypedDeterministicIdentity(const std::string &label) {
    const fv_surface_mesh<TT, II> empty;
    const auto cube = MakeTypedCube<TT, II>(static_cast<TT>(0), static_cast<TT>(0), static_cast<TT>(0),
                                           static_cast<TT>(1), static_cast<TT>(1), static_cast<TT>(1));
    const std::vector<std::pair<MeshBooleanOperation4, std::string>> operations = {
        { MeshBooleanOperation4::Union, "union" },
        { MeshBooleanOperation4::Subtraction, "subtraction" },
        { MeshBooleanOperation4::Exclusion, "exclusion" },
    };

    for(const auto &operation : operations) {
        const auto first = BooleanMeshOp4(cube, empty, operation.first);
        const auto first_bytes = TypedMeshBytes(first);
        RequireTypedPostconditions(first, label + " " + operation.second + " first result");
        for(int i = 0; i < 8; ++i) {
            const auto next = BooleanMeshOp4(cube, empty, operation.first);
            Require(TypedMeshBytes(next) == first_bytes,
                    label + " " + operation.second + ": nondeterministic output");
            RequireTypedPostconditions(next, label + " " + operation.second + " repeated result");
        }
    }

    Require(BooleanMeshOp4(cube, empty, MeshBooleanOperation4::Intersection).faces.empty(),
            label + " intersection with empty should be empty");
}

void RequireDeterministic(const Mesh &lhs,
                          const Mesh &rhs,
                          MeshBooleanOperation4 op,
                          const std::string &label) {
    try {
        const auto first = BooleanMeshOp4(lhs, rhs, op);
        const auto first_bytes = MeshBytes(first);
        RequirePostconditions(first, label + " first result");
        for(int i = 0; i < 24; ++i) {
            const auto next = BooleanMeshOp4(lhs, rhs, op);
            Require(MeshBytes(next) == first_bytes, label + ": nondeterministic output");
            RequirePostconditions(next, label + " repeated result");
        }
    } catch(const std::exception &e) {
        throw std::runtime_error(label + ": " + e.what());
    }
}

void RequireMeasure(const Mesh &mesh,
                    T expected_volume,
                    T expected_area,
                    const std::string &label) {
    RequireNear(Volume(mesh), expected_volume, volume_tol, label + " volume");
    RequireNear(mesh.surface_area(), expected_area, area_tol, label + " area");
}

struct OperationCase {
    std::string name;
    Mesh lhs;
    Mesh rhs;
    bool check_union = false;
    T union_volume = 0.0;
    T union_area = 0.0;
    bool check_intersection = false;
    T intersection_volume = 0.0;
    T intersection_area = 0.0;
    bool check_subtraction = false;
    T subtraction_volume = 0.0;
    T subtraction_area = 0.0;
    bool check_exclusion = false;
    T exclusion_volume = 0.0;
    T exclusion_area = 0.0;
};

void RunMeasuredCase(const OperationCase &c) {
    RequireDeterministic(c.lhs, c.rhs, MeshBooleanOperation4::Union, c.name + " union");
    RequireDeterministic(c.lhs, c.rhs, MeshBooleanOperation4::Intersection, c.name + " intersection");
    RequireDeterministic(c.lhs, c.rhs, MeshBooleanOperation4::Subtraction, c.name + " subtraction");
    RequireDeterministic(c.lhs, c.rhs, MeshBooleanOperation4::Exclusion, c.name + " exclusion");

    if(c.check_union) {
        RequireMeasure(BooleanUnion4(c.lhs, c.rhs), c.union_volume, c.union_area, c.name + " union");
    }
    if(c.check_intersection) {
        RequireMeasure(BooleanIntersection4(c.lhs, c.rhs), c.intersection_volume, c.intersection_area, c.name + " intersection");
    }
    if(c.check_subtraction) {
        RequireMeasure(BooleanSubtraction4(c.lhs, c.rhs), c.subtraction_volume, c.subtraction_area, c.name + " subtraction");
    }
    if(c.check_exclusion) {
        RequireMeasure(BooleanExclusion4(c.lhs, c.rhs), c.exclusion_volume, c.exclusion_area, c.name + " exclusion");
    }
}

void TestEmptyOperands() {
    const Mesh empty;
    const Mesh vertex_only_empty = MakeVertexOnlyEmptyMesh();
    const Mesh cube = MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

    RequireMeasure(BooleanUnion4(empty, cube), 1.0, 6.0, "empty union B");
    RequireMeasure(BooleanUnion4(cube, empty), 1.0, 6.0, "A union empty");
    Require(IsEmpty(BooleanIntersection4(empty, cube)), "empty intersection B must be empty");
    Require(IsEmpty(BooleanIntersection4(cube, empty)), "A intersection empty must be empty");
    Require(IsEmpty(BooleanSubtraction4(empty, cube)), "empty subtraction B must be empty");
    RequireMeasure(BooleanSubtraction4(cube, empty), 1.0, 6.0, "A subtraction empty");
    RequireMeasure(BooleanExclusion4(empty, cube), 1.0, 6.0, "empty exclusion B");
    RequireMeasure(BooleanExclusion4(cube, empty), 1.0, 6.0, "A exclusion empty");

    RequireMeasure(BooleanMeshOp4(cube, empty, MeshBooleanOperation4::Subtraction),
                   1.0, 6.0, "dispatcher A subtraction empty");

    RequireMeasure(BooleanUnion4(vertex_only_empty, cube), 1.0, 6.0, "vertex-only empty union B");
    RequireMeasure(BooleanUnion4(cube, vertex_only_empty), 1.0, 6.0, "A union vertex-only empty");
    Require(IsEmpty(BooleanIntersection4(vertex_only_empty, cube)), "vertex-only empty intersection B must be empty");
    Require(IsEmpty(BooleanSubtraction4(vertex_only_empty, cube)), "vertex-only empty subtraction B must be empty");
    RequireMeasure(BooleanSubtraction4(cube, vertex_only_empty), 1.0, 6.0, "A subtraction vertex-only empty");
}

void TestIdentityNormalization() {
    const Mesh empty;
    const Mesh cube_with_unused_vertex = MakeCubeWithUnusedVertex();

    const auto lhs_identity = BooleanUnion4(cube_with_unused_vertex, empty);
    Require(lhs_identity.vertices.size() == 8, "lhs identity result should drop unused input vertices");
    RequireMeasure(lhs_identity, 1.0, 6.0, "lhs identity normalized cube");

    const auto rhs_identity = BooleanUnion4(empty, cube_with_unused_vertex);
    Require(rhs_identity.vertices.size() == 8, "rhs identity result should drop unused input vertices");
    RequireMeasure(rhs_identity, 1.0, 6.0, "rhs identity normalized cube");

    const auto subtraction_identity = BooleanSubtraction4(cube_with_unused_vertex, empty);
    Require(subtraction_identity.vertices.size() == 8, "subtraction identity result should drop unused input vertices");
    RequireMeasure(subtraction_identity, 1.0, 6.0, "subtraction identity normalized cube");

    const auto exclusion_identity = BooleanExclusion4(cube_with_unused_vertex, empty);
    Require(exclusion_identity.vertices.size() == 8, "exclusion identity result should drop unused input vertices");
    RequireMeasure(exclusion_identity, 1.0, 6.0, "exclusion identity normalized cube");
}

Mesh MakeInvalidNonFiniteMesh() {
    Mesh mesh = MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    mesh.vertices[0].x = std::numeric_limits<T>::quiet_NaN();
    return mesh;
}

Mesh MakeInvalidVertexOnlyEmptyMesh() {
    Mesh mesh = MakeVertexOnlyEmptyMesh();
    mesh.vertices[0].x = std::numeric_limits<T>::quiet_NaN();
    return mesh;
}

void RequireInvalidArgumentContaining(const std::function<void()> &fn,
                                      const std::string &needle,
                                      const std::string &label) {
    try {
        fn();
    } catch(const std::invalid_argument &e) {
        Require(std::string(e.what()).find(needle) != std::string::npos,
                label + ": diagnostic did not contain '" + needle + "': " + e.what());
        return;
    }
    throw std::runtime_error(label + ": expected std::invalid_argument");
}

void TestWrapperDiagnostics() {
    const Mesh empty;
    const Mesh cube = MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    const Mesh invalid = MakeInvalidNonFiniteMesh();
    const Mesh invalid_vertex_only_empty = MakeInvalidVertexOnlyEmptyMesh();

    RequireInvalidArgumentContaining(
        [&]() {
            (void)BooleanMeshOp4(empty, empty, static_cast<MeshBooleanOperation4>(1234));
        },
        "invalid MeshBooleanOperation4",
        "invalid enum cast");

    RequireInvalidArgumentContaining(
        [&]() {
            (void)BooleanUnion4(invalid, cube);
        },
        "lhs operand",
        "lhs conversion context");

    RequireInvalidArgumentContaining(
        [&]() {
            (void)BooleanUnion4(cube, invalid);
        },
        "rhs operand",
        "rhs conversion context");

    RequireInvalidArgumentContaining(
        [&]() {
            (void)BooleanUnion4(invalid_vertex_only_empty, cube);
        },
        "lhs operand contains a non-finite vertex",
        "vertex-only empty validation");
}

Boolean4NormalizedInput<T, I> MakeBroadPhaseTriangle(const vec3<T> &a,
                                                     const vec3<T> &b,
                                                     const vec3<T> &c,
                                                     Boolean4Operand operand) {
    Boolean4NormalizedInput<T, I> input;
    input.mesh.vertices = { a, b, c };
    input.mesh.faces = { {0, 1, 2} };
    input.mesh.recreate_involved_face_index();
    input.source_faces = { {operand, 0} };
    return input;
}

void RequireOneBroadPhaseCandidate(const Boolean4NormalizedInput<T, I> &lhs,
                                   const Boolean4NormalizedInput<T, I> &rhs,
                                   const std::string &label) {
    const auto candidates = BuildBoolean4TrianglePairBroadPhase(lhs, rhs);
    Require(candidates.size() == 1, label + ": expected exactly one broad-phase candidate");
    Require(candidates.front().lhs_face_id == 0, label + ": unexpected lhs face id");
    Require(candidates.front().rhs_face_id == 0, label + ": unexpected rhs face id");
}

void TestBroadPhaseCandidates() {
    RequireOneBroadPhaseCandidate(
        MakeBroadPhaseTriangle(vec3<T>(0.0, 0.0, 0.0), vec3<T>(2.0, 0.0, 0.0), vec3<T>(0.0, 2.0, 0.0), Boolean4Operand::Lhs),
        MakeBroadPhaseTriangle(vec3<T>(1.0, 0.0, 0.0), vec3<T>(3.0, 0.0, 0.0), vec3<T>(1.0, 2.0, 0.0), Boolean4Operand::Rhs),
        "coplanar overlapping triangles");

    RequireOneBroadPhaseCandidate(
        MakeBroadPhaseTriangle(vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0), Boolean4Operand::Lhs),
        MakeBroadPhaseTriangle(vec3<T>(1.0, 0.0, 0.0), vec3<T>(2.0, 0.0, 0.0), vec3<T>(1.0, 1.0, 0.0), Boolean4Operand::Rhs),
        "edge-touching triangle boxes");

    RequireOneBroadPhaseCandidate(
        MakeBroadPhaseTriangle(vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0), Boolean4Operand::Lhs),
        MakeBroadPhaseTriangle(vec3<T>(1.0, 1.0, 0.0), vec3<T>(2.0, 1.0, 0.0), vec3<T>(1.0, 2.0, 0.0), Boolean4Operand::Rhs),
        "vertex-touching triangle boxes");

    const auto disjoint = BuildBoolean4TrianglePairBroadPhase(
        MakeBroadPhaseTriangle(vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0), Boolean4Operand::Lhs),
        MakeBroadPhaseTriangle(vec3<T>(2.0, 2.0, 0.0), vec3<T>(3.0, 2.0, 0.0), vec3<T>(2.0, 3.0, 0.0), Boolean4Operand::Rhs));
    Require(disjoint.empty(), "disjoint triangle boxes should not produce candidates");
}

using Tri = std::array<vec3<T>, 3>;

void RequireTriangleCase(const Tri &lhs,
                         const Tri &rhs,
                         Boolean4TriangleTriangleCase expected,
                         const std::string &label) {
    const auto actual = ClassifyBoolean4TriangleTriangle(lhs, rhs);
    Require(actual.kind == expected, label + ": unexpected triangle-triangle classification");
}

void TestTriangleTriangleClassification() {
    RequireTriangleCase(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0) },
        { vec3<T>(2.0, 0.0, 0.0), vec3<T>(3.0, 0.0, 0.0), vec3<T>(2.0, 1.0, 0.0) },
        Boolean4TriangleTriangleCase::CoplanarDisjoint,
        "coplanar disjoint triangles");

    RequireTriangleCase(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0) },
        { vec3<T>(0.25, 0.25, -1.0), vec3<T>(0.25, 0.25, 1.0), vec3<T>(0.25, 1.25, 0.0) },
        Boolean4TriangleTriangleCase::ProperSegmentIntersection,
        "proper segment intersection");

    RequireTriangleCase(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0) },
        { vec3<T>(1.0, 0.0, 0.0), vec3<T>(2.0, 0.0, 0.0), vec3<T>(1.0, 1.0, 1.0) },
        Boolean4TriangleTriangleCase::PointContact,
        "single point contact");

    RequireTriangleCase(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0) },
        { vec3<T>(0.25, 0.0, 0.0), vec3<T>(0.75, 0.0, 0.0), vec3<T>(0.50, 0.0, 1.0) },
        Boolean4TriangleTriangleCase::SharedEdgeOrPartialOverlappingEdge,
        "partial overlapping edge");

    RequireTriangleCase(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(2.0, 0.0, 0.0), vec3<T>(0.0, 2.0, 0.0) },
        { vec3<T>(0.5, 0.0, 0.0), vec3<T>(2.5, 0.0, 0.0), vec3<T>(0.5, 2.0, 0.0) },
        Boolean4TriangleTriangleCase::CoplanarOverlapArea,
        "coplanar area overlap");

    RequireTriangleCase(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0) },
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0) },
        Boolean4TriangleTriangleCase::IdenticalTriangles,
        "identical triangles");

    RequireTriangleCase(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0) },
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0), vec3<T>(1.0, 0.0, 0.0) },
        Boolean4TriangleTriangleCase::ReversedIdenticalTriangles,
        "reversed identical triangles");

    RequireTriangleCase(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0) },
        { vec3<T>(0.0, 0.0, 2.0), vec3<T>(1.0, 0.0, 2.0), vec3<T>(0.0, 1.0, 2.0) },
        Boolean4TriangleTriangleCase::Disjoint,
        "non-coplanar disjoint triangles");
}


T TriangleArea(const std::array<vec3<T>, 3> &tri) {
    return ((tri[1] - tri[0]).Cross(tri[2] - tri[0])).length() * 0.5;
}

T SplitFacetArea(const Boolean4SplitTriangleResult<T> &split,
                 const Boolean4ArrangementFacet<T> &facet) {
    return TriangleArea({ split.approximate_vertices.at(facet.vertex_ids[0]),
                          split.approximate_vertices.at(facet.vertex_ids[1]),
                          split.approximate_vertices.at(facet.vertex_ids[2]) });
}

bool SplitContainsUndirectedEdge(const Boolean4SplitTriangleResult<T> &split,
                                 const vec3<T> &a,
                                 const vec3<T> &b) {
    const auto a_key = Boolean4SnapKey(a);
    const auto b_key = Boolean4SnapKey(b);
    for(const auto &facet : split.facets) {
        for(size_t i = 0; i < 3UL; ++i) {
            const auto lhs_id = facet.vertex_ids[i];
            const auto rhs_id = facet.vertex_ids[(i + 1UL) % 3UL];
            const auto &lhs_key = split.vertices.at(lhs_id).key;
            const auto &rhs_key = split.vertices.at(rhs_id).key;
            const bool forward = !(lhs_key < a_key) && !(a_key < lhs_key)
                              && !(rhs_key < b_key) && !(b_key < rhs_key);
            const bool reverse = !(lhs_key < b_key) && !(b_key < lhs_key)
                              && !(rhs_key < a_key) && !(a_key < rhs_key);
            if(forward || reverse) {
                return true;
            }
        }
    }
    return false;
}

std::string SplitBytes(const Boolean4SplitTriangleResult<T> &split) {
    std::ostringstream os;
    os << std::setprecision(17);
    os << "vertices " << split.approximate_vertices.size() << '\n';
    for(const auto &v : split.approximate_vertices) {
        os << v.x << ' ' << v.y << ' ' << v.z << '\n';
    }
    os << "facets " << split.facets.size() << '\n';
    for(const auto &facet : split.facets) {
        os << facet.vertex_ids[0] << ' ' << facet.vertex_ids[1] << ' ' << facet.vertex_ids[2] << '\n';
    }
    return os.str();
}

void TestArrangementFacetSplitting() {
    const Tri tri = { vec3<T>(0.0, 0.0, 0.0),
                      vec3<T>(1.0, 0.0, 0.0),
                      vec3<T>(0.0, 1.0, 0.0) };
    const Boolean4SourceFaceRef source = { Boolean4Operand::Lhs, 7 };

    const auto unsplit = SplitBoolean4TriangleIntoArrangementFacets<T>(tri, source, {});
    Require(unsplit.facets.size() == 1, "unconstrained triangle should produce one facet");
    RequireNear(SplitFacetArea(unsplit, unsplit.facets.front()), 0.5, area_tol,
                "unconstrained split area");

    const std::vector<Boolean4ConstraintSegment<T>> constraints = {
        { vec3<T>(0.5, 0.0, 0.0), vec3<T>(0.0, 0.5, 0.0) },
    };
    const auto split = SplitBoolean4TriangleIntoArrangementFacets<T>(tri, source, constraints);
    Require(split.facets.size() == 3, "single boundary-to-boundary constraint should produce three triangulated facets");
    Require(SplitContainsUndirectedEdge(split, constraints.front().a, constraints.front().b),
            "split facets should preserve the constraint as an output edge");

    T total_area = 0.0;
    for(const auto &facet : split.facets) {
        total_area += SplitFacetArea(split, facet);
    }
    RequireNear(total_area, 0.5, area_tol, "split facets should preserve source triangle area");

    const auto repeat = SplitBoolean4TriangleIntoArrangementFacets<T>(tri, source, constraints);
    Require(SplitBytes(split) == SplitBytes(repeat), "triangle splitting should be byte-for-byte deterministic");

    const auto lhs = MakeBroadPhaseTriangle(tri[0], tri[1], tri[2], Boolean4Operand::Lhs);
    const auto rhs = MakeBroadPhaseTriangle(vec3<T>(0.25, 0.25, -1.0),
                                            vec3<T>(0.25, 0.25, 1.0),
                                            vec3<T>(0.25, 1.25, 0.0),
                                            Boolean4Operand::Rhs);
    const auto arrangement = BuildBoolean4PairArrangement(lhs, rhs);
    Require(arrangement.lhs.split_faces.size() == 1, "pair arrangement should split every lhs triangle");
    Require(arrangement.rhs.split_faces.size() == 1, "pair arrangement should split every rhs triangle");
    Require(arrangement.lhs.split_faces.front().facets.size() > 1,
            "pair arrangement should apply classifier constraints to lhs triangle");
    Require(arrangement.rhs.split_faces.front().facets.size() > 1,
            "pair arrangement should apply classifier constraints to rhs triangle");
}


Boolean4SplitTriangleResult<T> MakeSingleFacetSplit(const Tri &tri,
                                                     Boolean4Operand operand) {
    return SplitBoolean4TriangleIntoArrangementFacets<T>(tri, { operand, 0 }, {});
}

Boolean4ClassifiedFacet<T> ClassifySingleFacet(const Tri &tri,
                                               const Boolean4NormalizedInput<T, I> &opposite,
                                               Boolean4Operand operand = Boolean4Operand::Lhs) {
    const auto split = MakeSingleFacetSplit(tri, operand);
    Require(split.facets.size() == 1, "classification test triangle should produce one facet");
    return ClassifyBoolean4ArrangementFacet(split, 0, split.facets.front(), opposite);
}

void TestArrangementFacetClassification() {
    const auto cube = NormalizeAndValidateInput4(MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
                                                 "rhs", Boolean4Operand::Rhs);

    const auto outside = ClassifySingleFacet(
        { vec3<T>(2.0, 0.1, 0.1), vec3<T>(2.0, 0.4, 0.1), vec3<T>(2.0, 0.1, 0.4) },
        cube);
    Require(outside.location == Boolean4FacetLocation::Outside,
            "facet representative outside cube should classify outside");
    Require(outside.boundary_facing == Boolean4BoundaryFacing::None,
            "outside facet should have no boundary facing");

    const auto inside = ClassifySingleFacet(
        { vec3<T>(0.25, 0.25, 0.25), vec3<T>(0.50, 0.25, 0.25), vec3<T>(0.25, 0.50, 0.25) },
        cube);
    Require(inside.location == Boolean4FacetLocation::Inside,
            "facet representative inside cube should classify inside");

    const auto same_facing = ClassifySingleFacet(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(0.4, 0.4, 0.0), vec3<T>(0.4, 0.0, 0.0) },
        cube);
    Require(same_facing.location == Boolean4FacetLocation::OnBoundary,
            "coplanar cube-boundary facet should classify on-boundary");
    Require(same_facing.boundary_facing == Boolean4BoundaryFacing::SameFacing,
            "matching cube-boundary orientation should classify same-facing");

    const auto opposite_facing = ClassifySingleFacet(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(0.4, 0.0, 0.0), vec3<T>(0.4, 0.4, 0.0) },
        cube);
    Require(opposite_facing.location == Boolean4FacetLocation::OnBoundary,
            "reversed coplanar cube-boundary facet should classify on-boundary");
    Require(opposite_facing.boundary_facing == Boolean4BoundaryFacing::OppositeFacing,
            "reversed cube-boundary orientation should classify opposite-facing");

    Boolean4OperandArrangement<T> arrangement;
    arrangement.split_faces.push_back(MakeSingleFacetSplit(
        { vec3<T>(2.0, 0.1, 0.1), vec3<T>(2.0, 0.4, 0.1), vec3<T>(2.0, 0.1, 0.4) },
        Boolean4Operand::Lhs));
    arrangement.split_faces.push_back(MakeSingleFacetSplit(
        { vec3<T>(0.25, 0.25, 0.25), vec3<T>(0.50, 0.25, 0.25), vec3<T>(0.25, 0.50, 0.25) },
        Boolean4Operand::Lhs));
    const auto classified = ClassifyBoolean4OperandArrangementFacets(arrangement, cube);
    Require(classified.facets.size() == 2,
            "operand arrangement classification should emit every split facet");
    Require(classified.facets[0].location == Boolean4FacetLocation::Outside,
            "classified facets should remain in deterministic split-face order");
    Require(classified.facets[1].location == Boolean4FacetLocation::Inside,
            "second classified facet should preserve its inside classification");
}


Boolean4ClassifiedFacet<T> MakeClassifiedFacet(Boolean4Operand operand,
                                               uint64_t facet_id,
                                               Boolean4FacetLocation location,
                                               Boolean4BoundaryFacing boundary_facing = Boolean4BoundaryFacing::None) {
    Boolean4ClassifiedFacet<T> facet;
    facet.location = location;
    facet.boundary_facing = boundary_facing;
    facet.split_face_id = facet_id / 10;
    facet.facet_id = facet_id;
    facet.source_face = { operand, facet_id };
    return facet;
}

Boolean4OperandFacetClassification<T> MakeFacetClassification(Boolean4Operand operand) {
    Boolean4OperandFacetClassification<T> classification;
    classification.facets = {
        MakeClassifiedFacet(operand, 0, Boolean4FacetLocation::Outside),
        MakeClassifiedFacet(operand, 1, Boolean4FacetLocation::Inside),
        MakeClassifiedFacet(operand, 2, Boolean4FacetLocation::OnBoundary, Boolean4BoundaryFacing::SameFacing),
        MakeClassifiedFacet(operand, 3, Boolean4FacetLocation::OnBoundary, Boolean4BoundaryFacing::OppositeFacing),
    };
    return classification;
}

size_t CountEmittedFacets(const Boolean4OperandFacetDecisions<T> &decisions) {
    return static_cast<size_t>(std::count_if(decisions.facets.begin(), decisions.facets.end(),
        [](const auto &decision) { return decision.emit; }));
}

const Boolean4FacetDecision<T> &DecisionByFacetId(const Boolean4OperandFacetDecisions<T> &decisions,
                                                  uint64_t facet_id) {
    const auto it = std::find_if(decisions.facets.begin(), decisions.facets.end(),
        [&](const auto &decision) { return decision.facet_id == facet_id; });
    Require(it != decisions.facets.end(), "missing facet decision");
    return *it;
}

std::string DecisionsBytes(const Boolean4OperandFacetDecisions<T> &decisions) {
    std::ostringstream os;
    for(const auto &decision : decisions.facets) {
        os << static_cast<int>(decision.operand) << ' '
           << decision.source_face.face_id << ' '
           << decision.split_face_id << ' '
           << decision.facet_id << ' '
           << decision.emit << ' '
           << decision.reverse_orientation << ' '
           << static_cast<int>(decision.reason) << '\n';
    }
    return os.str();
}


Boolean4OperandFacetDecisions<T> MakeEmitAllDecisions(const Boolean4OperandArrangement<T> &arrangement,
                                                      Boolean4Operand operand,
                                                      bool reverse_orientation = false) {
    Boolean4OperandFacetDecisions<T> decisions;
    for(uint64_t split_face_id = 0; split_face_id < arrangement.split_faces.size(); ++split_face_id) {
        const auto &split = arrangement.split_faces[split_face_id];
        for(const auto &facet : split.facets) {
            Boolean4FacetDecision<T> decision;
            decision.emit = true;
            decision.reverse_orientation = reverse_orientation;
            decision.operand = operand;
            decision.split_face_id = split_face_id;
            decision.facet_id = facet.stable_id;
            decision.source_face = facet.source_face;
            decision.reason = operand == Boolean4Operand::Lhs
                            ? Boolean4FacetDecisionReason::LhsOutsideKept
                            : Boolean4FacetDecisionReason::RhsOutsideKept;
            decisions.facets.push_back(decision);
        }
    }
    std::sort(decisions.facets.begin(), decisions.facets.end());
    return decisions;
}


Boolean4OperandArrangement<T> MakeUnconstrainedArrangement(const Boolean4NormalizedInput<T, I> &input) {
    return BuildBoolean4OperandArrangement(input, std::vector<std::vector<Boolean4ConstraintSegment<T>>>(input.mesh.faces.size()));
}


void RequireRuntimeErrorContaining(const std::function<void()> &fn,
                                   const std::string &needle,
                                   const std::string &label) {
    try {
        fn();
    } catch(const std::runtime_error &e) {
        Require(std::string(e.what()).find(needle) != std::string::npos,
                label + ": diagnostic did not contain '" + needle + "': " + e.what());
        return;
    }
    throw std::runtime_error(label + ": expected std::runtime_error");
}


void TestOutputHalfedgeAssembly() {
    const auto cube_input = NormalizeAndValidateInput4(MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
                                                       "lhs", Boolean4Operand::Lhs);
    Boolean4PairArrangement<T> cube_arrangement;
    cube_arrangement.lhs = MakeUnconstrainedArrangement(cube_input);
    Boolean4PairFacetDecisions<T> cube_decisions;
    cube_decisions.lhs = MakeEmitAllDecisions(cube_arrangement.lhs, Boolean4Operand::Lhs);

    const auto cube = AssembleBoolean4OutputHalfedgeMesh<T, I>(cube_arrangement, cube_decisions);
    Require(cube.vertices.size() == 8, "halfedge assembly should merge cube vertices by topology key");
    Require(cube.faces.size() == 12, "halfedge assembly should emit all cube triangles");
    RequirePostconditions(cube, "halfedge-assembled cube");
    RequireMeasure(cube, 1.0, 6.0, "halfedge-assembled cube");

    const auto cube_repeat = AssembleBoolean4OutputHalfedgeMesh<T, I>(cube_arrangement, cube_decisions);
    Require(MeshBytes(cube) == MeshBytes(cube_repeat),
            "halfedge assembly should be byte-for-byte deterministic");

    Boolean4PairFacetDecisions<T> reversed_decisions;
    reversed_decisions.lhs = MakeEmitAllDecisions(cube_arrangement.lhs, Boolean4Operand::Lhs, true);
    const auto reversed_cube = AssembleBoolean4OutputHalfedgeMesh<T, I>(cube_arrangement, reversed_decisions);
    Require(SignedVolume(reversed_cube) < 0.0,
            "halfedge assembly should preserve requested reversed facet orientation");

    Boolean4PairArrangement<T> open_arrangement;
    open_arrangement.lhs.split_faces.push_back(MakeSingleFacetSplit(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0) },
        Boolean4Operand::Lhs));
    Boolean4PairFacetDecisions<T> open_decisions;
    open_decisions.lhs = MakeEmitAllDecisions(open_arrangement.lhs, Boolean4Operand::Lhs);
    RequireRuntimeErrorContaining(
        [&]() {
            (void)AssembleBoolean4OutputHalfedgeMesh<T, I>(open_arrangement, open_decisions);
        },
        "non-manifold edge",
        "open output facet assembly");
}


void TestOutputPostconditionVerification() {
    auto cube = MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    VerifyBoolean4OutputPostconditions(cube, MeshBooleanOperation4::Union);

    auto duplicate = cube;
    duplicate.faces.push_back(duplicate.faces.front());
    duplicate.recreate_involved_face_index();
    RequireRuntimeErrorContaining(
        [&]() {
            VerifyBoolean4OutputPostconditions(duplicate, MeshBooleanOperation4::Union);
        },
        "duplicate faces",
        "duplicate output face verification");

    Mesh open;
    open.vertices = { vec3<T>(0.0, 0.0, 0.0),
                      vec3<T>(1.0, 0.0, 0.0),
                      vec3<T>(0.0, 1.0, 0.0) };
    open.faces = { {0, 1, 2} };
    open.recreate_involved_face_index();
    RequireRuntimeErrorContaining(
        [&]() {
            VerifyBoolean4OutputPostconditions(open, MeshBooleanOperation4::Subtraction);
        },
        "subtraction output failed postcondition: not a closed manifold",
        "operation-named output verification");
}

void TestBooleanFacetSelectionTruthTables() {
    Boolean4PairFacetClassification<T> classification;
    classification.lhs = MakeFacetClassification(Boolean4Operand::Lhs);
    classification.rhs = MakeFacetClassification(Boolean4Operand::Rhs);

    const auto union_decisions = SelectBoolean4PairFacets(classification, MeshBooleanOperation4::Union);
    Require(CountEmittedFacets(union_decisions.lhs) == 2, "union should keep lhs outside and same-facing representative facets");
    Require(CountEmittedFacets(union_decisions.rhs) == 1, "union should keep only rhs outside facets");
    Require(DecisionByFacetId(union_decisions.lhs, 0).emit, "union should emit lhs outside facet");
    Require(DecisionByFacetId(union_decisions.lhs, 2).reason == Boolean4FacetDecisionReason::LhsSameBoundaryKeptAsRepresentative,
            "union should keep lhs same-facing coincident boundary as representative");
    Require(!DecisionByFacetId(union_decisions.rhs, 2).emit,
            "union should drop duplicate rhs same-facing coincident boundary");
    Require(!DecisionByFacetId(union_decisions.lhs, 3).emit && !DecisionByFacetId(union_decisions.rhs, 3).emit,
            "union should drop opposite-facing contact facets");

    const auto intersection_decisions = SelectBoolean4PairFacets(classification, MeshBooleanOperation4::Intersection);
    Require(CountEmittedFacets(intersection_decisions.lhs) == 2, "intersection should keep lhs inside and same-facing representative facets");
    Require(CountEmittedFacets(intersection_decisions.rhs) == 1, "intersection should keep only rhs inside facets");
    Require(DecisionByFacetId(intersection_decisions.lhs, 1).reason == Boolean4FacetDecisionReason::LhsInsideKept,
            "intersection should keep lhs inside facets");
    Require(DecisionByFacetId(intersection_decisions.rhs, 1).reason == Boolean4FacetDecisionReason::RhsInsideKept,
            "intersection should keep rhs inside facets");
    Require(!DecisionByFacetId(intersection_decisions.lhs, 3).emit && !DecisionByFacetId(intersection_decisions.rhs, 3).emit,
            "intersection should drop opposite-facing zero-thickness contact facets");

    const auto subtraction_decisions = SelectBoolean4PairFacets(classification, MeshBooleanOperation4::Subtraction);
    Require(CountEmittedFacets(subtraction_decisions.lhs) == 2, "subtraction should keep lhs outside and opposite-facing contact facets");
    Require(CountEmittedFacets(subtraction_decisions.rhs) == 1, "subtraction should keep only rhs inside facets");
    Require(DecisionByFacetId(subtraction_decisions.lhs, 3).reason == Boolean4FacetDecisionReason::LhsOppositeBoundaryKeptForSubtractionContact,
            "subtraction should preserve lhs boundary when rhs only touches it from outside");
    Require(DecisionByFacetId(subtraction_decisions.rhs, 1).reverse_orientation,
            "subtraction should reverse selected rhs facets");
    Require(!DecisionByFacetId(subtraction_decisions.lhs, 2).emit && !DecisionByFacetId(subtraction_decisions.rhs, 2).emit,
            "subtraction should drop same-facing coincident boundaries");

    const auto exclusion_decisions = SelectBoolean4PairFacets(classification, MeshBooleanOperation4::Exclusion);
    Require(CountEmittedFacets(exclusion_decisions.lhs) == 2, "exclusion should keep lhs outside and inside facets");
    Require(CountEmittedFacets(exclusion_decisions.rhs) == 2, "exclusion should keep rhs outside and inside facets");
    Require(DecisionByFacetId(exclusion_decisions.lhs, 1).reverse_orientation,
            "exclusion should reverse lhs facets inside rhs");
    Require(DecisionByFacetId(exclusion_decisions.rhs, 1).reverse_orientation,
            "exclusion should reverse rhs facets inside lhs");
    Require(!DecisionByFacetId(exclusion_decisions.lhs, 2).emit && !DecisionByFacetId(exclusion_decisions.rhs, 2).emit,
            "exclusion should drop same-facing coincident boundaries");
    Require(!DecisionByFacetId(exclusion_decisions.lhs, 3).emit && !DecisionByFacetId(exclusion_decisions.rhs, 3).emit,
            "exclusion should drop opposite-facing contact facets");

    const auto repeated = SelectBoolean4PairFacets(classification, MeshBooleanOperation4::Exclusion);
    Require(DecisionsBytes(exclusion_decisions.lhs) == DecisionsBytes(repeated.lhs),
            "lhs facet selection should be deterministic");
    Require(DecisionsBytes(exclusion_decisions.rhs) == DecisionsBytes(repeated.rhs),
            "rhs facet selection should be deterministic");
}

void TestDiagnosticsAndDebugDump() {
    const Boolean4ValidationDiagnostic validation = {
        "validation", Boolean4Operand::Rhs, 12, 3, "contains a non-finite vertex" };
    Require(Boolean4DiagnosticString(validation).find("operand=rhs") != std::string::npos,
            "validation diagnostic should include operand name");
    Require(Boolean4DiagnosticString(validation).find("face=12") != std::string::npos,
            "validation diagnostic should include face id");
    Require(Boolean4DiagnosticString(validation).find("vertex=3") != std::string::npos,
            "validation diagnostic should include vertex id");

    const Boolean4TrianglePairDiagnostic triangle_pair = {
        4, 9, Boolean4TriangleTriangleCase::ProperSegmentIntersection, 2 };
    const auto triangle_pair_text = Boolean4DiagnosticString(triangle_pair);
    Require(triangle_pair_text.find("lhs_face=4") != std::string::npos,
            "triangle-pair diagnostic should include lhs face id");
    Require(triangle_pair_text.find("rhs_face=9") != std::string::npos,
            "triangle-pair diagnostic should include rhs face id");
    Require(triangle_pair_text.find("proper segment intersection") != std::string::npos,
            "triangle-pair diagnostic should include exact predicate case name");

    Boolean4ArrangementSplitDiagnostic split_diag;
    split_diag.source_face = { Boolean4Operand::Lhs, 7 };
    split_diag.constraint_count = 1;
    split_diag.vertex_count = 4;
    split_diag.facet_count = 2;
    Require(Boolean4DiagnosticString(split_diag).find("source_face=7") != std::string::npos,
            "arrangement diagnostic should include source face id");

    const auto facet = MakeClassifiedFacet(Boolean4Operand::Rhs, 21, Boolean4FacetLocation::OnBoundary,
                                           Boolean4BoundaryFacing::OppositeFacing);
    const Boolean4FacetClassificationDiagnostic facet_diag = {
        Boolean4Operand::Rhs, facet.split_face_id, facet.facet_id, facet.source_face,
        facet.location, facet.boundary_facing };
    const auto facet_text = Boolean4DiagnosticString(facet_diag);
    Require(facet_text.find("operand=rhs") != std::string::npos,
            "facet diagnostic should include operand name");
    Require(facet_text.find("on-boundary") != std::string::npos,
            "facet diagnostic should include location name");
    Require(facet_text.find("opposite-facing") != std::string::npos,
            "facet diagnostic should include boundary facing name");

    const Boolean4OutputAssemblyDiagnostic output_diag = {
        5, 6, 7, Boolean4Operand::Lhs, "non-manifold edge" };
    const auto output_text = Boolean4DiagnosticString(output_diag);
    Require(output_text.find("output_face=5") != std::string::npos,
            "output diagnostic should include output face id");
    Require(output_text.find("output_vertex=6") != std::string::npos,
            "output diagnostic should include output vertex id");

    Boolean4PairArrangement<T> arrangement;
    arrangement.lhs.split_faces.push_back(MakeSingleFacetSplit(
        { vec3<T>(0.0, 0.0, 0.0), vec3<T>(1.0, 0.0, 0.0), vec3<T>(0.0, 1.0, 0.0) },
        Boolean4Operand::Lhs));
    Boolean4PairFacetClassification<T> classification;
    classification.lhs = MakeFacetClassification(Boolean4Operand::Lhs);
    classification.rhs = MakeFacetClassification(Boolean4Operand::Rhs);
    const auto decisions = SelectBoolean4PairFacets(classification, MeshBooleanOperation4::Union);

    const auto dump = Boolean4DebugDump(arrangement, classification, decisions, MeshBooleanOperation4::Union);
    const auto repeat = Boolean4DebugDump(arrangement, classification, decisions, MeshBooleanOperation4::Union);
    Require(dump == repeat, "debug dump should be byte-for-byte deterministic");
    Require(dump.find("operation union") != std::string::npos,
            "debug dump should include operation name");
    Require(dump.find("reason \"lhs outside kept\"") != std::string::npos,
            "debug dump should include facet decision reason names");
    Require(dump.find("source_face 0") != std::string::npos,
            "debug dump should include source face ids");
}

void TestPropertyAndFuzzIdentities() {
    const Mesh empty;
    const std::vector<uint32_t> seeds = { 1U, 7U, 19U, 31U, 73U, 127U };
    for(size_t i = 0; i < seeds.size(); ++i) {
        const auto a = MakeSeededConvexCube(seeds[i]);
        const auto b = MakeSeededConvexCube(seeds[(i + 1UL) % seeds.size()] + 1000U);
        const std::string label = "fixed-seed convex cube pair " + std::to_string(seeds[i]);

        const auto union_ab = BooleanUnion4(a, b);
        const auto union_ba = BooleanUnion4(b, a);
        RequireSameMeshBytes(union_ab, union_ba, label + " union commutativity");
        RequirePostconditions(union_ab, label + " union");

        const auto intersection_ab = BooleanIntersection4(a, b);
        const auto intersection_ba = BooleanIntersection4(b, a);
        RequireSameMeshBytes(intersection_ab, intersection_ba, label + " intersection commutativity");
        Require(IsEmpty(intersection_ab), label + ": disjoint generated cubes should not intersect");

        Require(IsEmpty(BooleanSubtraction4(a, a)), label + ": A subtraction A must be empty");
        Require(IsEmpty(BooleanExclusion4(a, a)), label + ": A exclusion A must be empty");
        RequireSameMeshBytes(BooleanUnion4(a, empty), BooleanUnion4(empty, a), label + " union empty identity");
        Require(IsEmpty(BooleanIntersection4(a, empty)), label + ": A intersection empty must be empty");
    }
}

void TestMetamorphicTransforms() {
    const Mesh empty;
    const auto a = MakeSeededConvexCube(4242U);
    const auto b = MakeSeededConvexCube(5252U);

    const vec3<T> delta(3.25, -2.5, 1.75);
    RequireSameMeshBytes(BooleanUnion4(TranslateMesh(a, delta), empty),
                         TranslateMesh(BooleanUnion4(a, empty), delta),
                         "translation metamorphic union identity");
    Require(IsEmpty(BooleanIntersection4(TranslateMesh(a, delta), TranslateMesh(b, delta))),
            "translation metamorphic disjoint intersection should remain empty");

    RequireSameCanonicalMeshBytes(BooleanSubtraction4(Rotate90ZMesh(a), empty),
                                  Rotate90ZMesh(BooleanSubtraction4(a, empty)),
                                  "rotation metamorphic subtraction identity");
    Require(IsEmpty(BooleanExclusion4(Rotate90ZMesh(a), Rotate90ZMesh(a))),
            "rotation metamorphic A exclusion A should remain empty");

    const T scale = 2.0;
    const auto scaled = BooleanUnion4(ScaleMesh(a, scale), empty);
    const auto baseline = BooleanUnion4(a, empty);
    RequireNear(Volume(scaled), Volume(baseline) * scale * scale * scale,
                volume_tol, "uniform scale metamorphic volume");
    RequireNear(scaled.surface_area(), baseline.surface_area() * scale * scale,
                area_tol, "uniform scale metamorphic surface area");
    RequirePostconditions(scaled, "uniform scale metamorphic output");
}

void TestNumericalRobustnessBoundaries() {
    RequireTypedDeterministicIdentity<float, uint32_t>("float uint32 explicit instantiation");
    RequireTypedDeterministicIdentity<float, uint64_t>("float uint64 explicit instantiation");
    RequireTypedDeterministicIdentity<double, uint32_t>("double uint32 explicit instantiation");
    RequireTypedDeterministicIdentity<double, uint64_t>("double uint64 explicit instantiation");

    RequireRuntimeErrorContaining(
        []() {
            (void)Boolean4CheckedIndex<uint8_t>(256UL, "vertices");
        },
        "too many vertices",
        "single vertex index overflow");
    RequireRuntimeErrorContaining(
        []() {
            Boolean4RequireCountFitsIndex<uint8_t>(257UL, "faces");
        },
        "too many faces",
        "face count overflow");
}

void TestBoolean4NoBspDependency() {
    std::ifstream in("src/YgorMeshesBoolean4.cc");
    Require(in.good(), "Boolean4 source should be readable for BSP dependency guard");

    const std::string source((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    Require(source.find("YgorMeshesBSPTree") == std::string::npos,
            "Boolean4 implementation must not include or reference YgorMeshesBSPTree");
    Require(source.find("bsp_tree") == std::string::npos,
            "Boolean4 implementation must not call bsp_tree helpers");
    Require(source.find("BSP") == std::string::npos,
            "Boolean4 implementation must not carry BSP-specific assumptions");
}

void TestOperationCases() {
    const std::vector<OperationCase> cases = {
        {"disjoint cubes",
         MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
         MakeCube(2.0, 0.0, 0.0, 3.0, 1.0, 1.0),
         true, 2.0, 12.0, true, 0.0, 0.0, true, 1.0, 6.0, true, 2.0, 12.0},
        {"overlapping cubes",
         MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
         MakeCube(0.5, 0.5, 0.5, 1.5, 1.5, 1.5),
         true, 1.875, 10.5, true, 0.125, 1.5, false, 0.0, 0.0, false, 0.0, 0.0},
        {"contained cubes",
         MakeCube(0.0, 0.0, 0.0, 2.0, 2.0, 2.0),
         MakeCube(0.5, 0.5, 0.5, 1.5, 1.5, 1.5),
         true, 8.0, 24.0, true, 1.0, 6.0, true, 7.0, 30.0, true, 7.0, 30.0},
        {"identical cubes",
         MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
         MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
         true, 1.0, 6.0, true, 1.0, 6.0, true, 0.0, 0.0, true, 0.0, 0.0},
        {"face-touching cubes",
         MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
         MakeCube(1.0, 0.0, 0.0, 2.0, 1.0, 1.0),
         true, 2.0, 10.0, true, 0.0, 0.0, true, 1.0, 6.0, true, 2.0, 10.0},
        {"edge-touching cubes",
         MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
         MakeCube(1.0, 1.0, 0.0, 2.0, 2.0, 1.0),
         false, 0.0, 0.0, true, 0.0, 0.0, true, 1.0, 6.0, false, 0.0, 0.0},
        {"vertex-touching cubes",
         MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
         MakeCube(1.0, 1.0, 1.0, 2.0, 2.0, 2.0),
         false, 0.0, 0.0, true, 0.0, 0.0, true, 1.0, 6.0, false, 0.0, 0.0},
        {"partial coplanar face overlap",
         MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
         MakeCube(0.5, 1.0, 0.0, 1.5, 2.0, 1.0),
         false, 0.0, 0.0, true, 0.0, 0.0, false, 0.0, 0.0, false, 0.0, 0.0},
        {"tetrahedron intersecting cube",
         MakeTetrahedron(),
         MakeCube(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
         false, 0.0, 0.0, false, 0.0, 0.0, false, 0.0, 0.0, false, 0.0, 0.0},
    };

    for(const auto &c : cases) {
        RunMeasuredCase(c);
    }
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"empty operand algebra", TestEmptyOperands},
        {"identity normalization", TestIdentityNormalization},
        {"wrapper diagnostics", TestWrapperDiagnostics},
        {"deterministic broad phase", TestBroadPhaseCandidates},
        {"triangle-triangle classification", TestTriangleTriangleClassification},
        {"arrangement facet splitting", TestArrangementFacetSplitting},
        {"arrangement facet classification", TestArrangementFacetClassification},
        {"boolean facet selection truth tables", TestBooleanFacetSelectionTruthTables},
        {"diagnostics and debug dump", TestDiagnosticsAndDebugDump},
        {"output halfedge assembly", TestOutputHalfedgeAssembly},
        {"output postcondition verification", TestOutputPostconditionVerification},
        {"property and deterministic fuzz identities", TestPropertyAndFuzzIdentities},
        {"metamorphic transforms", TestMetamorphicTransforms},
        {"numerical robustness boundaries", TestNumericalRobustnessBoundaries},
        {"no BSP dependency guard", TestBoolean4NoBspDependency},
        {"closed mesh operation cases", TestOperationCases},
    };

    int failures = 0;
    for(const auto &test : tests) {
        try {
            test.second();
            std::cout << "PASS " << test.first << '\n';
        } catch(const std::exception &e) {
            ++failures;
            std::cerr << "FAIL " << test.first << ": " << e.what() << '\n';
        }
    }

    return failures == 0 ? 0 : 1;
}
