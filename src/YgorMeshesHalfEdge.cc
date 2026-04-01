//YgorMeshesHalfEdge.cc - Written by hal clark in 2026.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorStats.h"
#include "YgorString.h"

#include "YgorMeshesHalfEdge.h"

//#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
//#endif


// ----- he_surface_mesh constructors ------------------------------------------

template <class T, class I>
he_surface_mesh<T,I>::he_surface_mesh(){ }

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template he_surface_mesh< float , uint32_t >::he_surface_mesh(void);
    template he_surface_mesh< float , uint64_t >::he_surface_mesh(void);

    template he_surface_mesh< double, uint32_t >::he_surface_mesh(void);
    template he_surface_mesh< double, uint64_t >::he_surface_mesh(void);
#endif


template <class T, class I>
he_surface_mesh<T,I>::he_surface_mesh(const he_surface_mesh<T,I> &in) :
    vertices(in.vertices),
    vertex_normals(in.vertex_normals),
    vertex_colours(in.vertex_colours),
    vertex_halfedges(in.vertex_halfedges),
    halfedges(in.halfedges),
    face_halfedges(in.face_halfedges),
    metadata(in.metadata) { }

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template he_surface_mesh< float , uint32_t >::he_surface_mesh(const he_surface_mesh< float , uint32_t > &);
    template he_surface_mesh< float , uint64_t >::he_surface_mesh(const he_surface_mesh< float , uint64_t > &);

    template he_surface_mesh< double, uint32_t >::he_surface_mesh(const he_surface_mesh< double, uint32_t > &);
    template he_surface_mesh< double, uint64_t >::he_surface_mesh(const he_surface_mesh< double, uint64_t > &);
#endif


// ----- he_surface_mesh operators ---------------------------------------------

template <class T, class I>
he_surface_mesh<T,I> &
he_surface_mesh<T,I>::operator=(const he_surface_mesh<T,I> &rhs){
    if(this == &rhs) return *this;
    this->vertices         = rhs.vertices;
    this->vertex_normals   = rhs.vertex_normals;
    this->vertex_colours   = rhs.vertex_colours;
    this->vertex_halfedges = rhs.vertex_halfedges;
    this->halfedges        = rhs.halfedges;
    this->face_halfedges   = rhs.face_halfedges;
    this->metadata         = rhs.metadata;
    return *this;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template he_surface_mesh<float , uint32_t > &
             he_surface_mesh<float , uint32_t >::operator=(const he_surface_mesh<float , uint32_t > &);
    template he_surface_mesh<float , uint64_t > &
             he_surface_mesh<float , uint64_t >::operator=(const he_surface_mesh<float , uint64_t > &);

    template he_surface_mesh<double, uint32_t > &
             he_surface_mesh<double, uint32_t >::operator=(const he_surface_mesh<double, uint32_t > &);
    template he_surface_mesh<double, uint64_t > &
             he_surface_mesh<double, uint64_t >::operator=(const he_surface_mesh<double, uint64_t > &);
#endif


template <class T, class I>
bool
he_surface_mesh<T,I>::operator==(const he_surface_mesh<T,I> &rhs) const {
    if(this == &rhs) return true;
    return (this->vertices         == rhs.vertices)
        && (this->vertex_normals   == rhs.vertex_normals)
        && (this->vertex_colours   == rhs.vertex_colours)
        && (this->vertex_halfedges == rhs.vertex_halfedges)
        && (this->halfedges.size() == rhs.halfedges.size())
        && ([&](){
               for(size_t i = 0; i < this->halfedges.size(); ++i){
                   const auto &a = this->halfedges[i];
                   const auto &b = rhs.halfedges[i];
                   if(a.vertex != b.vertex) return false;
                   if(a.face   != b.face)   return false;
                   if(a.twin   != b.twin)   return false;
                   if(a.next   != b.next)   return false;
                   if(a.prev   != b.prev)   return false;
               }
               return true;
           }())
        && (this->face_halfedges   == rhs.face_halfedges)
        && (this->metadata         == rhs.metadata);
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template bool he_surface_mesh<float , uint32_t>::operator==(const he_surface_mesh<float , uint32_t> &) const;
    template bool he_surface_mesh<float , uint64_t>::operator==(const he_surface_mesh<float , uint64_t> &) const;

    template bool he_surface_mesh<double, uint32_t>::operator==(const he_surface_mesh<double, uint32_t> &) const;
    template bool he_surface_mesh<double, uint64_t>::operator==(const he_surface_mesh<double, uint64_t> &) const;
#endif


template <class T, class I>
bool
he_surface_mesh<T,I>::operator!=(const he_surface_mesh<T,I> &rhs) const {
    return !(*this == rhs);
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template bool he_surface_mesh<float , uint32_t>::operator!=(const he_surface_mesh<float , uint32_t> &) const;
    template bool he_surface_mesh<float , uint64_t>::operator!=(const he_surface_mesh<float , uint64_t> &) const;

    template bool he_surface_mesh<double, uint32_t>::operator!=(const he_surface_mesh<double, uint32_t> &) const;
    template bool he_surface_mesh<double, uint64_t>::operator!=(const he_surface_mesh<double, uint64_t> &) const;
#endif


// ----- he_surface_mesh swap --------------------------------------------------

template <class T, class I>
void
he_surface_mesh<T,I>::swap(he_surface_mesh<T,I> &in){
    if(this == &in) return;
    std::swap(this->vertices        , in.vertices);
    std::swap(this->vertex_normals  , in.vertex_normals);
    std::swap(this->vertex_colours  , in.vertex_colours);
    std::swap(this->vertex_halfedges, in.vertex_halfedges);
    std::swap(this->halfedges       , in.halfedges);
    std::swap(this->face_halfedges  , in.face_halfedges);
    std::swap(this->metadata        , in.metadata);
    return;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template void he_surface_mesh<float , uint32_t >::swap(he_surface_mesh<float , uint32_t> &);
    template void he_surface_mesh<float , uint64_t >::swap(he_surface_mesh<float , uint64_t> &);

    template void he_surface_mesh<double, uint32_t >::swap(he_surface_mesh<double, uint32_t> &);
    template void he_surface_mesh<double, uint64_t >::swap(he_surface_mesh<double, uint64_t> &);
#endif


// ----- he_surface_mesh colour packing ----------------------------------------

template <class T, class I>
uint32_t
he_surface_mesh<T,I>::pack_RGBA32_colour(std::array<uint8_t,4> c) const {
    uint32_t out = 0U;
    out |= (static_cast<uint32_t>(c[0]) << 24U);
    out |= (static_cast<uint32_t>(c[1]) << 16U);
    out |= (static_cast<uint32_t>(c[2]) <<  8U);
    out |= (static_cast<uint32_t>(c[3]) <<  0U);
    return out;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template uint32_t he_surface_mesh<float , uint32_t>::pack_RGBA32_colour(std::array<uint8_t,4>) const;
    template uint32_t he_surface_mesh<float , uint64_t>::pack_RGBA32_colour(std::array<uint8_t,4>) const;

    template uint32_t he_surface_mesh<double, uint32_t>::pack_RGBA32_colour(std::array<uint8_t,4>) const;
    template uint32_t he_surface_mesh<double, uint64_t>::pack_RGBA32_colour(std::array<uint8_t,4>) const;
#endif


template <class T, class I>
std::array<uint8_t,4>
he_surface_mesh<T,I>::unpack_RGBA32_colour(uint32_t c) const {
    std::array<uint8_t,4> out;
    out[0] = static_cast<uint8_t>((c >> 24U) & 0xFFU);
    out[1] = static_cast<uint8_t>((c >> 16U) & 0xFFU);
    out[2] = static_cast<uint8_t>((c >>  8U) & 0xFFU);
    out[3] = static_cast<uint8_t>((c >>  0U) & 0xFFU);
    return out;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template std::array<uint8_t,4> he_surface_mesh<float , uint32_t>::unpack_RGBA32_colour(uint32_t) const;
    template std::array<uint8_t,4> he_surface_mesh<float , uint64_t>::unpack_RGBA32_colour(uint32_t) const;

    template std::array<uint8_t,4> he_surface_mesh<double, uint32_t>::unpack_RGBA32_colour(uint32_t) const;
    template std::array<uint8_t,4> he_surface_mesh<double, uint64_t>::unpack_RGBA32_colour(uint32_t) const;
#endif


// ----- he_surface_mesh surface_area ------------------------------------------

template <class T, class I>
T
he_surface_mesh<T,I>::surface_area(int64_t n) const {
    const auto N_faces = static_cast<int64_t>(this->face_halfedges.size());

    int64_t f_begin = 0;
    int64_t f_end   = N_faces;
    if(static_cast<int64_t>(0) <= n){
        if(N_faces <= n){
            throw std::invalid_argument("Selected face does not exist. Cannot continue.");
        }
        f_begin = n;
        f_end   = n + 1;
    }

    Stats::Running_Sum<T> rs_sarea;
    for(int64_t f = f_begin; f < f_end; ++f){
        // Collect face vertices.
        const auto verts = this->face_vertices(static_cast<I>(f));
        if(verts.size() < 3) continue;
        if(verts.size() > 3){
            throw std::runtime_error("Encountered facet with more than 3 vertices."
                                     " Cannot compute surface area for non-triangular faces.");
        }

        const auto P_A = this->vertices.at( verts[0] );
        const auto P_B = this->vertices.at( verts[1] );
        const auto P_C = this->vertices.at( verts[2] );

        const auto R_BA = (P_B - P_A);
        const auto R_CA = (P_C - P_A);

        const auto C = R_BA.Cross( R_CA );
        const auto surf_area = (C.length() / static_cast<T>(2));
        rs_sarea.Digest(surf_area);
    }

    return rs_sarea.Current_Sum();
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template float  he_surface_mesh<float , uint32_t >::surface_area(int64_t) const;
    template float  he_surface_mesh<float , uint64_t >::surface_area(int64_t) const;

    template double he_surface_mesh<double, uint32_t >::surface_area(int64_t) const;
    template double he_surface_mesh<double, uint64_t >::surface_area(int64_t) const;
#endif


// ----- he_surface_mesh compute_vertex_normals --------------------------------

template <class T, class I>
void
he_surface_mesh<T,I>::compute_vertex_normals(void){
    this->vertex_normals.assign(this->vertices.size(), vec3<T>(static_cast<T>(0),
                                                               static_cast<T>(0),
                                                               static_cast<T>(0)));
    const auto N_faces = this->face_halfedges.size();
    for(size_t f = 0; f < N_faces; ++f){
        const auto verts = this->face_vertices(static_cast<I>(f));
        if(verts.size() != 3){
            throw std::runtime_error("Non-triangular face detected in compute_vertex_normals(): face "
                + std::to_string(static_cast<uint64_t>(f))
                + " has " + std::to_string(static_cast<uint64_t>(verts.size()))
                + " vertices (expected 3).");
        }

        const auto P_A = this->vertices.at( verts[0] );
        const auto P_B = this->vertices.at( verts[1] );
        const auto P_C = this->vertices.at( verts[2] );

        const auto R_BA = (P_B - P_A);
        const auto R_CA = (P_C - P_A);
        const auto C = R_BA.Cross( R_CA );

        // The cross product magnitude is twice the triangle area; using it as
        // the weight gives area-weighted normals.
        if(C.length() < static_cast<T>(1E-30)) continue;

        for(const auto &vi : verts){
            this->vertex_normals.at(vi) += C;
        }
    }

    for(auto &vn : this->vertex_normals){
        const auto len = vn.length();
        if(len > static_cast<T>(1E-30)){
            vn /= len;
        }
    }
    return;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template void he_surface_mesh<float , uint32_t >::compute_vertex_normals(void);
    template void he_surface_mesh<float , uint64_t >::compute_vertex_normals(void);

    template void he_surface_mesh<double, uint32_t >::compute_vertex_normals(void);
    template void he_surface_mesh<double, uint64_t >::compute_vertex_normals(void);
#endif


// ----- he_surface_mesh element counts ----------------------------------------

template <class T, class I>
I
he_surface_mesh<T,I>::num_vertices(void) const {
    return static_cast<I>(this->vertices.size());
}

template <class T, class I>
I
he_surface_mesh<T,I>::num_faces(void) const {
    return static_cast<I>(this->face_halfedges.size());
}

template <class T, class I>
I
he_surface_mesh<T,I>::num_halfedges(void) const {
    return static_cast<I>(this->halfedges.size());
}

template <class T, class I>
I
he_surface_mesh<T,I>::num_edges(void) const {
    // Each interior edge contributes two half-edges (twins); each boundary edge
    // contributes one half-edge with twin == sentinel.  So:
    //   num_halfedges = 2 * interior_edges + boundary_edges
    //   total_edges   = interior_edges + boundary_edges
    I boundary = static_cast<I>(0);
    for(const auto &he : this->halfedges){
        if(he.twin == sentinel) ++boundary;
    }
    // interior half-edges = num_halfedges - boundary
    // interior_edges      = interior half-edges / 2
    // total_edges         = interior_edges + boundary
    const auto n_he = static_cast<I>(this->halfedges.size());
    return (n_he - boundary) / static_cast<I>(2) + boundary;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template uint32_t he_surface_mesh<float , uint32_t >::num_vertices(void) const;
    template uint64_t he_surface_mesh<float , uint64_t >::num_vertices(void) const;
    template uint32_t he_surface_mesh<double, uint32_t >::num_vertices(void) const;
    template uint64_t he_surface_mesh<double, uint64_t >::num_vertices(void) const;

    template uint32_t he_surface_mesh<float , uint32_t >::num_faces(void) const;
    template uint64_t he_surface_mesh<float , uint64_t >::num_faces(void) const;
    template uint32_t he_surface_mesh<double, uint32_t >::num_faces(void) const;
    template uint64_t he_surface_mesh<double, uint64_t >::num_faces(void) const;

    template uint32_t he_surface_mesh<float , uint32_t >::num_halfedges(void) const;
    template uint64_t he_surface_mesh<float , uint64_t >::num_halfedges(void) const;
    template uint32_t he_surface_mesh<double, uint32_t >::num_halfedges(void) const;
    template uint64_t he_surface_mesh<double, uint64_t >::num_halfedges(void) const;

    template uint32_t he_surface_mesh<float , uint32_t >::num_edges(void) const;
    template uint64_t he_surface_mesh<float , uint64_t >::num_edges(void) const;
    template uint32_t he_surface_mesh<double, uint32_t >::num_edges(void) const;
    template uint64_t he_surface_mesh<double, uint64_t >::num_edges(void) const;
#endif


// ----- he_surface_mesh adjacency queries -------------------------------------

template <class T, class I>
std::vector<I>
he_surface_mesh<T,I>::vertex_faces(I vert) const {
    std::vector<I> out;
    if(vert >= static_cast<I>(this->vertex_halfedges.size())) return out;

    const I start = this->vertex_halfedges.at(vert);
    if(start == sentinel) return out;

    I cur = start;
    do {
        const auto &he = this->halfedges.at(cur);
        if(he.face != sentinel){
            out.push_back(he.face);
        }
        // Move to the next outgoing half-edge around the vertex:
        // go to the previous half-edge in the same face, then take its twin.
        const I prev_he = he.prev;
        const I twin_of_prev = this->halfedges.at(prev_he).twin;
        if(twin_of_prev == sentinel){
            // Reached a boundary; we cannot continue the full fan.
            break;
        }
        cur = twin_of_prev;
    } while(cur != start);

    // If we broke out early (boundary), also walk the other direction.
    if(cur != start){
        // Walk from start in the opposite direction.
        I cur2 = start;
        for(;;){
            const auto &he2 = this->halfedges.at(cur2);
            const I tw = he2.twin;
            if(tw == sentinel) break;
            const I nxt = this->halfedges.at(tw).next;
            if(nxt == start) break;
            cur2 = nxt;
            const auto &he3 = this->halfedges.at(cur2);
            if(he3.face != sentinel){
                out.push_back(he3.face);
            }
        }
    }

    return out;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template std::vector<uint32_t> he_surface_mesh<float , uint32_t>::vertex_faces(uint32_t) const;
    template std::vector<uint64_t> he_surface_mesh<float , uint64_t>::vertex_faces(uint64_t) const;

    template std::vector<uint32_t> he_surface_mesh<double, uint32_t>::vertex_faces(uint32_t) const;
    template std::vector<uint64_t> he_surface_mesh<double, uint64_t>::vertex_faces(uint64_t) const;
#endif


template <class T, class I>
std::vector<I>
he_surface_mesh<T,I>::vertex_neighbours(I vert) const {
    std::vector<I> out;
    if(vert >= static_cast<I>(this->vertex_halfedges.size())) return out;

    const I start = this->vertex_halfedges.at(vert);
    if(start == sentinel) return out;

    auto add_neighbour = [&](I v){
        if(v != vert){
            out.push_back(v);
        }
    };

    I cur = start;
    bool hit_boundary = false;

    // Walk around the vertex using prev->twin, collecting both adjacent
    // face vertices (from he.next and he.prev) at each step.
    do {
        const auto &he = this->halfedges.at(cur);

        // Neighbour at the end of this half-edge within the face.
        const auto &next_he = this->halfedges.at(he.next);
        add_neighbour(next_he.vertex);

        // Neighbour at the beginning of the previous half-edge within the face.
        const auto &prev_he = this->halfedges.at(he.prev);
        add_neighbour(prev_he.vertex);

        const I twin_of_prev = prev_he.twin;
        if(twin_of_prev == sentinel){
            hit_boundary = true;
            break;
        }
        cur = twin_of_prev;
    } while(cur != start);

    // If we broke out early (boundary), also walk the other direction
    // around the vertex using twin->next.
    if(hit_boundary){
        I cur2 = start;
        for(;;){
            const auto &he2 = this->halfedges.at(cur2);
            if(he2.twin == sentinel) break;

            cur2 = this->halfedges.at(he2.twin).next;
            if(cur2 == start) break;

            const auto &he3 = this->halfedges.at(cur2);

            const auto &next_he3 = this->halfedges.at(he3.next);
            add_neighbour(next_he3.vertex);

            const auto &prev_he3 = this->halfedges.at(he3.prev);
            add_neighbour(prev_he3.vertex);
        }
    }

    // Deduplicate neighbours.
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template std::vector<uint32_t> he_surface_mesh<float , uint32_t>::vertex_neighbours(uint32_t) const;
    template std::vector<uint64_t> he_surface_mesh<float , uint64_t>::vertex_neighbours(uint64_t) const;

    template std::vector<uint32_t> he_surface_mesh<double, uint32_t>::vertex_neighbours(uint32_t) const;
    template std::vector<uint64_t> he_surface_mesh<double, uint64_t>::vertex_neighbours(uint64_t) const;
#endif


template <class T, class I>
std::vector<I>
he_surface_mesh<T,I>::face_vertices(I face) const {
    std::vector<I> out;
    if(face >= static_cast<I>(this->face_halfedges.size())) return out;

    const I start = this->face_halfedges.at(face);
    I cur = start;
    do {
        const auto &he = this->halfedges.at(cur);
        out.push_back(he.vertex);
        cur = he.next;
    } while(cur != start);

    return out;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template std::vector<uint32_t> he_surface_mesh<float , uint32_t>::face_vertices(uint32_t) const;
    template std::vector<uint64_t> he_surface_mesh<float , uint64_t>::face_vertices(uint64_t) const;

    template std::vector<uint32_t> he_surface_mesh<double, uint32_t>::face_vertices(uint32_t) const;
    template std::vector<uint64_t> he_surface_mesh<double, uint64_t>::face_vertices(uint64_t) const;
#endif


template <class T, class I>
std::vector<I>
he_surface_mesh<T,I>::face_neighbours(I face) const {
    std::vector<I> out;
    if(face >= static_cast<I>(this->face_halfedges.size())) return out;

    const I start = this->face_halfedges.at(face);
    I cur = start;
    do {
        const auto &he = this->halfedges.at(cur);
        if(he.twin != sentinel){
            const auto &twin_he = this->halfedges.at(he.twin);
            if(twin_he.face != sentinel){
                out.push_back(twin_he.face);
            }
        }
        cur = he.next;
    } while(cur != start);

    return out;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template std::vector<uint32_t> he_surface_mesh<float , uint32_t>::face_neighbours(uint32_t) const;
    template std::vector<uint64_t> he_surface_mesh<float , uint64_t>::face_neighbours(uint64_t) const;

    template std::vector<uint32_t> he_surface_mesh<double, uint32_t>::face_neighbours(uint32_t) const;
    template std::vector<uint64_t> he_surface_mesh<double, uint64_t>::face_neighbours(uint64_t) const;
#endif


// ----- he_surface_mesh boundary queries --------------------------------------

template <class T, class I>
bool
he_surface_mesh<T,I>::is_boundary_vertex(I vert) const {
    if(vert >= static_cast<I>(this->vertex_halfedges.size())) return false;

    const I start = this->vertex_halfedges.at(vert);
    if(start == sentinel) return true; // Isolated vertex.

    I cur = start;
    do {
        const auto &he = this->halfedges.at(cur);
        if(he.twin == sentinel) return true;

        const I prev_he = he.prev;
        const I twin_of_prev = this->halfedges.at(prev_he).twin;
        if(twin_of_prev == sentinel) return true;
        cur = twin_of_prev;
    } while(cur != start);

    return false;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template bool he_surface_mesh<float , uint32_t>::is_boundary_vertex(uint32_t) const;
    template bool he_surface_mesh<float , uint64_t>::is_boundary_vertex(uint64_t) const;

    template bool he_surface_mesh<double, uint32_t>::is_boundary_vertex(uint32_t) const;
    template bool he_surface_mesh<double, uint64_t>::is_boundary_vertex(uint64_t) const;
#endif


template <class T, class I>
bool
he_surface_mesh<T,I>::is_boundary_halfedge(I he) const {
    if(he >= static_cast<I>(this->halfedges.size())) return false;
    return (this->halfedges.at(he).twin == sentinel)
        || (this->halfedges.at(he).face == sentinel);
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template bool he_surface_mesh<float , uint32_t>::is_boundary_halfedge(uint32_t) const;
    template bool he_surface_mesh<float , uint64_t>::is_boundary_halfedge(uint64_t) const;

    template bool he_surface_mesh<double, uint32_t>::is_boundary_halfedge(uint32_t) const;
    template bool he_surface_mesh<double, uint64_t>::is_boundary_halfedge(uint64_t) const;
#endif


// ----- he_surface_mesh convert_to_fv_surface_mesh ----------------------------

template <class T, class I>
fv_surface_mesh<T,I>
he_surface_mesh<T,I>::convert_to_fv_surface_mesh(void) const {
    fv_surface_mesh<T,I> fvsm;
    fvsm.vertices       = this->vertices;
    fvsm.vertex_normals = this->vertex_normals;
    fvsm.vertex_colours = this->vertex_colours;
    fvsm.metadata       = this->metadata;

    const auto N_faces = this->face_halfedges.size();
    fvsm.faces.reserve(N_faces);
    for(size_t f = 0; f < N_faces; ++f){
        fvsm.faces.push_back( this->face_vertices(static_cast<I>(f)) );
    }

    return fvsm;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh<float , uint32_t> he_surface_mesh<float , uint32_t>::convert_to_fv_surface_mesh(void) const;
    template fv_surface_mesh<float , uint64_t> he_surface_mesh<float , uint64_t>::convert_to_fv_surface_mesh(void) const;

    template fv_surface_mesh<double, uint32_t> he_surface_mesh<double, uint32_t>::convert_to_fv_surface_mesh(void) const;
    template fv_surface_mesh<double, uint64_t> he_surface_mesh<double, uint64_t>::convert_to_fv_surface_mesh(void) const;
#endif


// ----- he_surface_mesh metadata ----------------------------------------------

template <class T, class I>
bool
he_surface_mesh<T,I>::MetadataKeyPresent(std::string key) const {
    return (this->metadata.find(key) != this->metadata.end());
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template bool he_surface_mesh<float , uint32_t>::MetadataKeyPresent(std::string key) const;
    template bool he_surface_mesh<float , uint64_t>::MetadataKeyPresent(std::string key) const;

    template bool he_surface_mesh<double, uint32_t>::MetadataKeyPresent(std::string key) const;
    template bool he_surface_mesh<double, uint64_t>::MetadataKeyPresent(std::string key) const;
#endif


template <class T, class I>
template <class U>
std::optional<U>
he_surface_mesh<T,I>::GetMetadataValueAs(std::string key) const {
    const auto metadata_cit = this->metadata.find(key);
    if( (metadata_cit == this->metadata.end())  || !Is_String_An_X<U>(metadata_cit->second) ){
        return std::optional<U>();
    }else{
        return std::make_optional(stringtoX<U>(metadata_cit->second));
    }
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template std::optional<int32_t> he_surface_mesh<float , uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<int32_t> he_surface_mesh<float , uint64_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<int32_t> he_surface_mesh<double, uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<int32_t> he_surface_mesh<double, uint64_t>::GetMetadataValueAs(std::string key) const;

    template std::optional<uint32_t> he_surface_mesh<float , uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<uint32_t> he_surface_mesh<float , uint64_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<uint32_t> he_surface_mesh<double, uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<uint32_t> he_surface_mesh<double, uint64_t>::GetMetadataValueAs(std::string key) const;

    template std::optional<int64_t> he_surface_mesh<float , uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<int64_t> he_surface_mesh<float , uint64_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<int64_t> he_surface_mesh<double, uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<int64_t> he_surface_mesh<double, uint64_t>::GetMetadataValueAs(std::string key) const;

    template std::optional<uint64_t> he_surface_mesh<float , uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<uint64_t> he_surface_mesh<float , uint64_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<uint64_t> he_surface_mesh<double, uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<uint64_t> he_surface_mesh<double, uint64_t>::GetMetadataValueAs(std::string key) const;

    template std::optional<float> he_surface_mesh<float , uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<float> he_surface_mesh<float , uint64_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<float> he_surface_mesh<double, uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<float> he_surface_mesh<double, uint64_t>::GetMetadataValueAs(std::string key) const;

    template std::optional<double> he_surface_mesh<float , uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<double> he_surface_mesh<float , uint64_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<double> he_surface_mesh<double, uint32_t>::GetMetadataValueAs(std::string key) const;
    template std::optional<double> he_surface_mesh<double, uint64_t>::GetMetadataValueAs(std::string key) const;
#endif


// ----- convert_fv_to_he_surface_mesh (free function) -------------------------

template <class T, class I>
he_surface_mesh<T,I>
convert_fv_to_he_surface_mesh(const fv_surface_mesh<T,I> &fvsm){
    he_surface_mesh<T,I> out;
    constexpr I sentinel = he_surface_mesh<T,I>::sentinel;

    out.vertices       = fvsm.vertices;
    out.vertex_normals = fvsm.vertex_normals;
    out.vertex_colours = fvsm.vertex_colours;
    out.metadata       = fvsm.metadata;

    const auto N_verts = fvsm.vertices.size();
    const auto N_faces = fvsm.faces.size();

    out.vertex_halfedges.assign(N_verts, sentinel);
    out.face_halfedges.resize(N_faces);

    // First pass: count the total number of half-edges needed.
    size_t total_he = 0;
    for(const auto &fv : fvsm.faces){
        total_he += fv.size();
    }
    out.halfedges.resize(total_he);

    // Validate that the total counts fit in the index type I.
    if(N_verts > static_cast<size_t>(sentinel)
    || N_faces > static_cast<size_t>(sentinel)
    || total_he > static_cast<size_t>(sentinel)){
        throw std::runtime_error("Mesh size exceeds the capacity of the index type: "
            + std::to_string(N_verts) + " vertices, "
            + std::to_string(N_faces) + " faces, "
            + std::to_string(total_he) + " half-edges.");
    }

    // Second pass: create half-edges for each face and link next/prev within
    // each face loop.  Also build an edge map for twin lookup.
    //
    // The map key is (v_from, v_to) for a directed edge.
    std::map<std::pair<I,I>, size_t> edge_to_he;

    size_t he_idx = 0;
    for(size_t f = 0; f < N_faces; ++f){
        const auto &fv = fvsm.faces[f];
        const auto n = fv.size();
        if(n != 3){
            throw std::runtime_error("Non-triangular face detected: face "
                + std::to_string(static_cast<uint64_t>(f))
                + " has " + std::to_string(static_cast<uint64_t>(n))
                + " vertices (exactly 3 required). Cannot build half-edge mesh.");
        }

        // Validate that no vertex appears more than once within this face
        // (which would create self-loop / degenerate edges).
        for(size_t a = 0; a < n; ++a){
            for(size_t b = a + 1; b < n; ++b){
                if(fv[a] == fv[b]){
                    throw std::runtime_error("Degenerate face detected: face "
                        + std::to_string(static_cast<uint64_t>(f))
                        + " has duplicate vertex index "
                        + std::to_string(static_cast<uint64_t>(fv[a]))
                        + ". Cannot build half-edge mesh.");
                }
            }
        }

        const size_t first_he = he_idx;
        out.face_halfedges[f] = static_cast<I>(first_he);

        for(size_t j = 0; j < n; ++j){
            auto &he = out.halfedges[he_idx];

            // Validate vertex index before using it.
            const I v_idx = fv[j];
            if(static_cast<size_t>(v_idx) >= out.vertices.size()){
                throw std::runtime_error(
                    "Face " + std::to_string(static_cast<uint64_t>(f))
                    + " references invalid vertex index "
                    + std::to_string(static_cast<uint64_t>(v_idx))
                    + " (vertex count: "
                    + std::to_string(static_cast<uint64_t>(out.vertices.size()))
                    + ").");
            }

            he.vertex = v_idx;
            he.face   = static_cast<I>(f);
            he.twin   = sentinel;
            he.next   = static_cast<I>(first_he + ((j + 1) % n));
            he.prev   = static_cast<I>(first_he + ((j + n - 1) % n));

            // Record outgoing half-edge for this vertex.
            if(out.vertex_halfedges[v_idx] == sentinel){
                out.vertex_halfedges[v_idx] = static_cast<I>(he_idx);
            }

            // Insert into edge map for twin lookup.
            const I v_from = v_idx;
            const I v_to   = fv[(j + 1) % n];
            auto key = std::make_pair(v_from, v_to);

            auto [it, inserted] = edge_to_he.emplace(key, he_idx);
            if(!inserted){
                throw std::runtime_error("Non-manifold edge detected: directed edge ("
                    + std::to_string(static_cast<uint64_t>(v_from)) + " -> "
                    + std::to_string(static_cast<uint64_t>(v_to))
                    + ") appears in multiple faces. Cannot build half-edge mesh.");
            }

            ++he_idx;
        }
    }

    // Third pass: link twins.
    for(auto &[key, idx] : edge_to_he){
        auto twin_key = std::make_pair(key.second, key.first);
        auto it = edge_to_he.find(twin_key);
        if(it != edge_to_he.end()){
            out.halfedges[idx].twin              = static_cast<I>(it->second);
            out.halfedges[it->second].twin       = static_cast<I>(idx);
        }
        // If no twin exists, this is a boundary edge and twin remains sentinel.
    }

    // For boundary vertices, ensure the stored half-edge is a boundary half-edge
    // (one whose twin is sentinel) so that the vertex fan walk starts from the
    // correct side.  A single O(H) scan over all half-edges suffices.
    for(size_t h = 0; h < out.halfedges.size(); ++h){
        if(out.halfedges[h].twin == sentinel){
            out.vertex_halfedges[out.halfedges[h].vertex] = static_cast<I>(h);
        }
    }

    // Validate that no vertex has a non-manifold (bow-tie) topology.
    //
    // For each vertex, walk the full one-ring fan.  If the number of half-edges
    // originating from the vertex that we encounter during the fan walk does not
    // equal the total number of half-edges originating from that vertex, then
    // the vertex has multiple disconnected fans (a bow-tie) and is non-manifold.
    {
        // Count total outgoing half-edges per vertex.
        std::vector<I> total_outgoing(N_verts, static_cast<I>(0));
        for(const auto &he : out.halfedges){
            total_outgoing.at(he.vertex) += static_cast<I>(1);
        }

        for(size_t v = 0; v < N_verts; ++v){
            const I start = out.vertex_halfedges[v];
            if(start == sentinel) continue;

            I fan_count = static_cast<I>(0);
            I cur = start;

            // Walk using prev->twin (one direction around the fan).
            bool hit_boundary = false;
            do {
                ++fan_count;
                const auto &he = out.halfedges.at(cur);
                const I twin_of_prev = out.halfedges.at(he.prev).twin;
                if(twin_of_prev == sentinel){
                    hit_boundary = true;
                    break;
                }
                cur = twin_of_prev;
            } while(cur != start);

            // If we hit a boundary, also walk the other direction (twin->next).
            if(hit_boundary){
                cur = start;
                for(;;){
                    const auto &he = out.halfedges.at(cur);
                    if(he.twin == sentinel) break;
                    cur = out.halfedges.at(he.twin).next;
                    if(cur == start) break;
                    ++fan_count;
                }
            }

            if(fan_count != total_outgoing[v]){
                throw std::runtime_error("Non-manifold vertex detected: vertex "
                    + std::to_string(static_cast<uint64_t>(v))
                    + " has multiple disconnected edge fans (bow-tie topology)."
                    " Cannot build half-edge mesh.");
            }
        }
    }

    return out;
}

#ifndef YGOR_MESHES_HALF_EDGE_DISABLE_ALL_SPECIALIZATIONS
    template he_surface_mesh<float , uint32_t>
    convert_fv_to_he_surface_mesh(const fv_surface_mesh<float , uint32_t> &);
    template he_surface_mesh<float , uint64_t>
    convert_fv_to_he_surface_mesh(const fv_surface_mesh<float , uint64_t> &);

    template he_surface_mesh<double, uint32_t>
    convert_fv_to_he_surface_mesh(const fv_surface_mesh<double, uint32_t> &);
    template he_surface_mesh<double, uint64_t>
    convert_fv_to_he_surface_mesh(const fv_surface_mesh<double, uint64_t> &);
#endif
