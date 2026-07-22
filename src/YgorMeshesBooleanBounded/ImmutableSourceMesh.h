#pragma once

#include "CanonicalBytes.h"
#include "ContractVersions.h"
#include "Outcome.h"
#include "PublicMeshReadView.h"
#include "Sha256.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace ygor::mesh_boolean::bounded {
template<class T> using scalar_bits_t = std::conditional_t<sizeof(T)==4,std::uint32_t,std::uint64_t>;
template<class T, class I>
class immutable_source_mesh {
  public:
    operand_id operand() const noexcept { return operand_; }
    std::size_t vertex_count() const noexcept { return coordinate_bits_.size()/3; }
    std::size_t face_count() const noexcept { return face_offsets_.empty() ? 0 : face_offsets_.size()-1; }
    const std::vector<scalar_bits_t<T>> &coordinate_bits() const noexcept { return coordinate_bits_; }
    const std::vector<std::uint64_t> &face_offsets() const noexcept { return face_offsets_; }
    const std::vector<I> &indices() const noexcept { return indices_; }
    const std::vector<std::uint8_t> &canonical_bytes() const noexcept { return bytes_; }
    const bounded_boolean_digest &digest() const noexcept { return digest_; }
  private:
    operand_id operand_ = operand_id::a;
    std::vector<scalar_bits_t<T>> coordinate_bits_;
    std::vector<std::uint64_t> face_offsets_;
    std::vector<I> indices_;
    std::vector<std::uint8_t> bytes_;
    bounded_boolean_digest digest_{};
    template<class U, class J> friend boolean_outcome<immutable_source_mesh<U,J>> capture_source(public_mesh_read_view<U,J>, operand_id);
};

inline bounded_boolean_error source_error(std::uint32_t subcode, bounded_boolean_error_category category = bounded_boolean_error_category::resource_limit) {
    bounded_boolean_error error;
    error.category = category; error.subcode = subcode; error.stage = static_cast<std::uint16_t>(stage_id::source_capture);
    error.summary = "immutable source capture failed";
    return error;
}

template<class T, class I>
boolean_outcome<immutable_source_mesh<T,I>> capture_source(public_mesh_read_view<T,I> view, operand_id operand) {
    static_assert(std::is_same_v<T,float> || std::is_same_v<T,double>);
    static_assert(std::is_same_v<I,std::uint32_t> || std::is_same_v<I,std::uint64_t>);
    std::uint64_t coordinate_count = 0, index_count = 0;
    if (!checked_multiply<std::uint64_t>(view.vertex_count(), 3, coordinate_count)) return boolean_outcome<immutable_source_mesh<T,I>>::failure(source_error(1101));
    for (std::size_t f=0; f<view.face_count(); ++f) if (!checked_add<std::uint64_t>(index_count, view.ring_size(f), index_count)) return boolean_outcome<immutable_source_mesh<T,I>>::failure(source_error(1102));
    immutable_source_mesh<T,I> out;
    out.operand_=operand;
    try {
        out.coordinate_bits_.reserve(static_cast<std::size_t>(coordinate_count));
        out.face_offsets_.reserve(view.face_count()+1);
        out.indices_.reserve(static_cast<std::size_t>(index_count));
        for (std::size_t v=0; v<view.vertex_count(); ++v) {
            const auto *p=view.vertex(v);
            for (T value : std::array<T,3>{{p->x,p->y,p->z}}) { scalar_bits_t<T> bits=0; std::memcpy(&bits,&value,sizeof(bits)); out.coordinate_bits_.push_back(bits); }
        }
        out.face_offsets_.push_back(0);
        for (std::size_t f=0; f<view.face_count(); ++f) {
            for (std::size_t i=0; i<view.ring_size(f); ++i) out.indices_.push_back(*view.index(f,i));
            out.face_offsets_.push_back(out.indices_.size());
        }
        canonical_writer w;
        w.u32(0x31534259); w.u16(contract_versions::source_snapshot); w.u8(static_cast<std::uint8_t>(operand));
        w.u8(sizeof(T)); w.u8(sizeof(I)); w.u64(out.vertex_count());
        for (auto bits:out.coordinate_bits_) { if constexpr(sizeof(T)==4) w.u32(bits); else w.u64(bits); }
        w.u64(out.face_count()); for(auto off:out.face_offsets_) w.u64(off);
        w.u64(out.indices_.size()); for(auto i:out.indices_) { if constexpr(sizeof(I)==4) w.u32(i); else w.u64(i); }
        out.bytes_=w.take(); out.digest_=sha256::digest(out.bytes_);
    } catch (const std::bad_alloc &) { return boolean_outcome<immutable_source_mesh<T,I>>::failure(source_error(1201)); }
      catch (const std::length_error &) { return boolean_outcome<immutable_source_mesh<T,I>>::failure(source_error(1202)); }
    return boolean_outcome<immutable_source_mesh<T,I>>::success(std::move(out));
}

template<class T,class I>
bool verify_source(const immutable_source_mesh<T,I> &source) {
    if (source.face_offsets().empty() || source.face_offsets().front()!=0 || source.face_offsets().back()!=source.indices().size()) return false;
    for(std::size_t i=1;i<source.face_offsets().size();++i) if(source.face_offsets()[i]<source.face_offsets()[i-1]) return false;
    return sha256::digest(source.canonical_bytes()) == source.digest();
}
}
