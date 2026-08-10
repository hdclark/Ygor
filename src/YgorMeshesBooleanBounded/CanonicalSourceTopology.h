#pragma once

#include "CanonicalBytes.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct canonical_source_topology_result final {
    std::vector<std::uint64_t> canonical_to_source_vertex;
    std::vector<std::uint64_t> source_to_canonical_vertex;
    std::vector<std::vector<std::uint64_t>> rings;
    std::vector<std::uint64_t> canonical_to_source_facet;
    std::uint64_t branches = 0;
};

namespace canonical_source_detail {
struct candidate {
    std::vector<std::uint8_t> bytes;
    std::vector<std::uint64_t> vertices;
    std::vector<std::vector<std::uint64_t>> rings;
    std::vector<std::uint64_t> facets;
};

inline std::vector<std::uint8_t> encode_candidate(
    const std::vector<std::uint64_t> &vertices,
    const std::vector<std::array<std::uint64_t,3>> &labels,
    const std::vector<std::vector<std::uint64_t>> &rings) {
    canonical_writer writer;
    writer.u64(vertices.size());
    for(auto source : vertices) for(auto word : labels[source]) writer.u64(word);
    writer.u64(rings.size());
    for(const auto &ring : rings) { writer.u64(ring.size()); for(auto v : ring) writer.u64(v); }
    return writer.take();
}
}

// An oriented manifold is fully labeled by a directed seed corner. Exhausting all
// seed corners avoids caller-order tie breaks, including exact graph automorphisms.
inline canonical_source_topology_result canonicalize_source_topology(
    std::size_t source_vertex_count,
    const std::vector<std::array<std::uint64_t,3>> &labels,
    const std::vector<std::vector<std::uint64_t>> &source_rings) {
    using edge_key=std::pair<std::uint64_t,std::uint64_t>;
    std::map<edge_key,std::vector<std::pair<std::uint64_t,std::uint64_t>>> edge_uses;
    for(std::uint64_t f=0;f<source_rings.size();++f) for(std::uint64_t c=0;c<source_rings[f].size();++c) {
        auto a=source_rings[f][c],b=source_rings[f][(c+1)%source_rings[f].size()];
        edge_uses[{std::min(a,b),std::max(a,b)}].push_back({f,c});
    }
    std::vector<std::vector<std::uint64_t>> adjacency(source_rings.size());
    for(const auto &entry:edge_uses) if(entry.second.size()==2) {
        auto a=entry.second[0].first,b=entry.second[1].first; adjacency[a].push_back(b);adjacency[b].push_back(a);
    }
    std::vector<bool> assigned(source_rings.size(),false);
    std::vector<canonical_source_detail::candidate> components;
    std::uint64_t branches=0;
    for(std::uint64_t initial=0;initial<source_rings.size();++initial) if(!assigned[initial]) {
        std::vector<std::uint64_t> component;std::queue<std::uint64_t> discover;discover.push(initial);assigned[initial]=true;
        while(!discover.empty()){auto f=discover.front();discover.pop();component.push_back(f);for(auto n:adjacency[f])if(!assigned[n]){assigned[n]=true;discover.push(n);}}
        canonical_source_detail::candidate best;bool have=false;
        for(auto seed:component) for(std::uint64_t shift=0;shift<source_rings[seed].size();++shift) {
            ++branches;
            std::map<std::uint64_t,std::uint64_t> vertex_ids;std::vector<std::uint64_t> vertices;
            std::set<std::uint64_t> seen_faces;std::queue<std::pair<std::uint64_t,std::uint64_t>> queue;queue.push({seed,shift});seen_faces.insert(seed);
            std::vector<std::vector<std::uint64_t>> rings;std::vector<std::uint64_t> facets;
            while(!queue.empty()) {
                auto current=queue.front();queue.pop();const auto &source_ring=source_rings[current.first];
                std::vector<std::uint64_t> ring;ring.reserve(source_ring.size());
                for(std::uint64_t i=0;i<source_ring.size();++i){auto source_v=source_ring[(current.second+i)%source_ring.size()];auto it=vertex_ids.find(source_v);if(it==vertex_ids.end()){auto id=vertices.size();vertex_ids[source_v]=id;vertices.push_back(source_v);ring.push_back(id);}else ring.push_back(it->second);}
                facets.push_back(current.first);rings.push_back(ring);
                for(std::uint64_t i=0;i<source_ring.size();++i){auto a=source_ring[(current.second+i)%source_ring.size()],b=source_ring[(current.second+i+1)%source_ring.size()];const auto &uses=edge_uses[{std::min(a,b),std::max(a,b)}];if(uses.size()!=2)continue;auto other=uses[0].first==current.first?uses[1]:uses[0];if(seen_faces.insert(other.first).second)queue.push({other.first,other.second});}
            }
            auto bytes=canonical_source_detail::encode_candidate(vertices,labels,rings);
            if(!have||bytes<best.bytes){best={std::move(bytes),std::move(vertices),std::move(rings),std::move(facets)};have=true;}
        }
        components.push_back(std::move(best));
    }
    std::sort(components.begin(),components.end(),[](const auto&a,const auto&b){return a.bytes<b.bytes;});
    canonical_source_topology_result out;out.source_to_canonical_vertex.assign(source_vertex_count,std::numeric_limits<std::uint64_t>::max());out.branches=branches;
    for(auto &component:components){auto base=out.canonical_to_source_vertex.size();for(auto v:component.vertices){out.source_to_canonical_vertex[v]=out.canonical_to_source_vertex.size();out.canonical_to_source_vertex.push_back(v);}for(auto &ring:component.rings){for(auto &v:ring)v+=base;out.rings.push_back(std::move(ring));}out.canonical_to_source_facet.insert(out.canonical_to_source_facet.end(),component.facets.begin(),component.facets.end());}
    return out;
}

} // namespace ygor::mesh_boolean::bounded
