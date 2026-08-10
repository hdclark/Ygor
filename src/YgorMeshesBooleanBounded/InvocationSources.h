#pragma once

#include "ImmutableSourceMesh.h"

namespace ygor::mesh_boolean::bounded {
template<class T,class I>
struct immutable_invocation_sources {
    immutable_source_mesh<T,I> a;
    immutable_source_mesh<T,I> b;
    bounded_boolean_digest digest{};
};
template<class T,class I>
boolean_outcome<immutable_invocation_sources<T,I>> capture_invocation_sources(const fv_surface_mesh<T,I> &a, const fv_surface_mesh<T,I> &b) {
    auto ca=capture_source(public_mesh_read_view<T,I>(a),operand_id::a); if(!ca.has_value()) return boolean_outcome<immutable_invocation_sources<T,I>>::failure(*ca.error());
    auto cb=capture_source(public_mesh_read_view<T,I>(b),operand_id::b); if(!cb.has_value()) return boolean_outcome<immutable_invocation_sources<T,I>>::failure(*cb.error());
    immutable_invocation_sources<T,I> out{std::move(*ca.value()),std::move(*cb.value()),{}};
    sha256 h; h.update(out.a.canonical_bytes()); h.update(out.b.canonical_bytes()); out.digest=h.finish();
    return boolean_outcome<immutable_invocation_sources<T,I>>::success(std::move(out));
}
}
