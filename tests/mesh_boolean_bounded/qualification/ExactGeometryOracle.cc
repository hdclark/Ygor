#include "ExactGeometryOracle.h"

#include <stdexcept>

namespace ygor::mesh_boolean::qualification {

ExactRational determinant2(const ExactRational& a00, const ExactRational& a01,
                           const ExactRational& a10, const ExactRational& a11) {
    return a00 * a11 - a01 * a10;
}

ExactRational determinant2(const std::array<std::array<ExactRational, 2>, 2>& m) {
    return determinant2(m[0][0], m[0][1], m[1][0], m[1][1]);
}

ExactRational determinant3(const ExactRational& a00, const ExactRational& a01,
                           const ExactRational& a02, const ExactRational& a10,
                           const ExactRational& a11, const ExactRational& a12,
                           const ExactRational& a20, const ExactRational& a21,
                           const ExactRational& a22) {
    return a00 * determinant2(a11, a12, a21, a22) -
           a01 * determinant2(a10, a12, a20, a22) +
           a02 * determinant2(a10, a11, a20, a21);
}

ExactRational determinant3(const std::array<std::array<ExactRational, 3>, 3>& m) {
    return determinant3(m[0][0], m[0][1], m[0][2],
                        m[1][0], m[1][1], m[1][2],
                        m[2][0], m[2][1], m[2][2]);
}

ExactRational orient2d_determinant(const ExactPoint2& a, const ExactPoint2& b,
                                   const ExactPoint2& c) {
    return determinant2(b.x - a.x, b.y - a.y, c.x - a.x, c.y - a.y);
}

int orient2d(const ExactPoint2& a, const ExactPoint2& b, const ExactPoint2& c) {
    return orient2d_determinant(a, b, c).sign();
}

ExactRational orient3d_determinant(const ExactPoint3& a, const ExactPoint3& b,
                                   const ExactPoint3& c, const ExactPoint3& d) {
    return determinant3(a.x - d.x, a.y - d.y, a.z - d.z,
                        b.x - d.x, b.y - d.y, b.z - d.z,
                        c.x - d.x, c.y - d.y, c.z - d.z);
}

int orient3d(const ExactPoint3& a, const ExactPoint3& b,
             const ExactPoint3& c, const ExactPoint3& d) {
    return orient3d_determinant(a, b, c, d).sign();
}

ExactRational dot(const ExactVector2& a, const ExactVector2& b) {
    return a.x * b.x + a.y * b.y;
}

ExactRational dot(const ExactVector3& a, const ExactVector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

ExactVector3 cross(const ExactVector3& a, const ExactVector3& b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

ExactRational point_plane_value(const ExactPoint3& point, const ExactVector3& normal,
                                const ExactRational& offset) {
    return dot(point, normal) + offset;
}

int point_plane_side(const ExactPoint3& point, const ExactVector3& normal,
                     const ExactRational& offset) {
    return point_plane_value(point, normal, offset).sign();
}

std::size_t select_projection_axis(const ExactVector3& normal) {
    if (normal.x.is_zero() && normal.y.is_zero() && normal.z.is_zero())
        throw std::domain_error("cannot project using a zero normal");
    const auto x = normal.x.abs();
    const auto y = normal.y.abs();
    const auto z = normal.z.abs();
    if (x >= y && x >= z) return 0;
    if (y >= z) return 1;
    return 2;
}

ExactPoint2 project_drop_axis(const ExactPoint3& point, std::size_t axis) {
    switch (axis) {
    case 0: return {point.y, point.z};
    case 1: return {point.x, point.z};
    case 2: return {point.x, point.y};
    default: throw std::out_of_range("projection axis must be 0, 1, or 2");
    }
}

} // namespace ygor::mesh_boolean::qualification
