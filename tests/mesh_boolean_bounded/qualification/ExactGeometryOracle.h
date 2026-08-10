#pragma once

#include "ExactRational.h"

#include <array>
#include <cstddef>

namespace ygor::mesh_boolean::qualification {

struct ExactPoint2 {
    ExactRational x;
    ExactRational y;
};

struct ExactPoint3 {
    ExactRational x;
    ExactRational y;
    ExactRational z;
};

using ExactVector2 = ExactPoint2;
using ExactVector3 = ExactPoint3;

ExactRational determinant2(const ExactRational& a00, const ExactRational& a01,
                           const ExactRational& a10, const ExactRational& a11);
ExactRational determinant2(const std::array<std::array<ExactRational, 2>, 2>& matrix);
ExactRational determinant3(const ExactRational& a00, const ExactRational& a01,
                           const ExactRational& a02, const ExactRational& a10,
                           const ExactRational& a11, const ExactRational& a12,
                           const ExactRational& a20, const ExactRational& a21,
                           const ExactRational& a22);
ExactRational determinant3(const std::array<std::array<ExactRational, 3>, 3>& matrix);

ExactRational orient2d_determinant(const ExactPoint2& a, const ExactPoint2& b,
                                   const ExactPoint2& c);
int orient2d(const ExactPoint2& a, const ExactPoint2& b, const ExactPoint2& c);
ExactRational orient3d_determinant(const ExactPoint3& a, const ExactPoint3& b,
                                   const ExactPoint3& c, const ExactPoint3& d);
int orient3d(const ExactPoint3& a, const ExactPoint3& b,
             const ExactPoint3& c, const ExactPoint3& d);

ExactRational dot(const ExactVector2& a, const ExactVector2& b);
ExactRational dot(const ExactVector3& a, const ExactVector3& b);
ExactVector3 cross(const ExactVector3& a, const ExactVector3& b);
ExactRational point_plane_value(const ExactPoint3& point, const ExactVector3& normal,
                                const ExactRational& offset);
int point_plane_side(const ExactPoint3& point, const ExactVector3& normal,
                     const ExactRational& offset);

// Returns the coordinate to drop. Equal absolute normal components prefer x, then y, then z.
std::size_t select_projection_axis(const ExactVector3& normal);
ExactPoint2 project_drop_axis(const ExactPoint3& point, std::size_t axis);

class ExactGeometryOracle {
public:
    static ExactRational determinant_2x2(
        const std::array<std::array<ExactRational, 2>, 2>& matrix) {
        return determinant2(matrix);
    }
    static ExactRational determinant_3x3(
        const std::array<std::array<ExactRational, 3>, 3>& matrix) {
        return determinant3(matrix);
    }
    static ExactRational orientation_2d_value(const ExactPoint2& a, const ExactPoint2& b,
                                              const ExactPoint2& c) {
        return orient2d_determinant(a, b, c);
    }
    static int orientation_2d(const ExactPoint2& a, const ExactPoint2& b,
                              const ExactPoint2& c) {
        return orient2d(a, b, c);
    }
    static ExactRational orientation_3d_value(const ExactPoint3& a, const ExactPoint3& b,
                                              const ExactPoint3& c, const ExactPoint3& d) {
        return orient3d_determinant(a, b, c, d);
    }
    static int orientation_3d(const ExactPoint3& a, const ExactPoint3& b,
                              const ExactPoint3& c, const ExactPoint3& d) {
        return orient3d(a, b, c, d);
    }
    static int plane_side(const ExactPoint3& point, const ExactVector3& normal,
                          const ExactRational& offset) {
        return point_plane_side(point, normal, offset);
    }
    static std::size_t projection_axis(const ExactVector3& normal) {
        return select_projection_axis(normal);
    }
};

} // namespace ygor::mesh_boolean::qualification
