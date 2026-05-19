//YgorMathVoronoi.h

#pragma once
#ifndef YGOR_MATH_VORONOI_H_
#define YGOR_MATH_VORONOI_H_

#include <cstdint>
#include <optional>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// Construct the planar Voronoi diagram dual to the input sites using Steven Fortune's sweep-line algorithm.
//
// The input sites are vec2's representing planar x-y coordinates. The returned diagram stores only generated
// Voronoi vertices explicitly; cells and edges refer back to the original input sites by index so that site
// coordinates are not duplicated in the output.
//
// Invalid input (for example, fewer than 2 sites, non-finite coordinates, or coincident sites) and unexpected
// sweep-line failures are reported by throwing an exception with a diagnostic message.
//
// The implementation follows the sweep-line construction described primarily in:
//  - Fortune S. A sweepline algorithm for Voronoi diagrams. Algorithmica. 1987;2:153-174.
//
// Additional references that informed the data-structure organization and degeneracy discussion are:
//  - de Berg M, van Kreveld M, Overmars M, Schwarzkopf O. Computational Geometry: Algorithms and Applications.
//    3rd ed. Springer; 2008. Chapter 7.
//  - Shewchuk JR. Robust adaptive floating-point geometric predicates. Proceedings of the twelfth annual symposium
//    on Computational geometry. 1996. pp. 141-150.
//  - Edelsbrunner H, Mücke EP. Simulation of simplicity: a technique to cope with degenerate cases in geometric
//    algorithms. ACM Transactions on Graphics. 1990;9(1):66-104.
//
// Robust orientation decisions use adaptive predicates via orient_sign and incircle_sign. Exact symbolic
// perturbation is not implemented; instead, coincident sites are rejected explicitly and near-degenerate circle
// events are filtered conservatively.
template <class T, class I>
struct VoronoiDiagram2 {
    struct Vertex {
        vec2<T> position;
        std::vector<I> incident_sites;
    };

    struct Edge {
        I left_site = 0;
        I right_site = 0;
        std::optional<I> vertex0;
        std::optional<I> vertex1;
        vec2<T> sample_point;
        vec2<T> direction;
    };

    std::vector<Vertex> vertices;
    std::vector<Edge> edges;
    std::vector<std::vector<I>> cell_edges;
};


template <class T, class I>
VoronoiDiagram2<T, I>
Voronoi_Diagram_2(const std::vector<vec2<T>> &verts);


#endif // YGOR_MATH_VORONOI_H_
