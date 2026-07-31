#pragma once
#ifndef YGOR_MESHES_BOOLEAN_VERIFICATION_H_
#define YGOR_MESHES_BOOLEAN_VERIFICATION_H_
#include "YgorMeshesBooleanContract.h"
#include <map>

namespace ygor { namespace mesh_boolean {

using verifier_callback=status_or<verification_report>(*)(const artifact_view&,const verification_spec&,const verification_environment_view&) noexcept;

struct verifier_registration{
    artifact_slot slot=artifact_slot::validated_operands;
    std::uint64_t artifact_type_tag=0;
    std::uint16_t artifact_schema=1,checker_version=1;
    std::vector<invariant_code>mandatory,exhaustive;
    verifier_callback callback=nullptr;
};

enum class dependency_node_kind:std::uint8_t{artifact,source_vertex,vertex,facet,edge,shell,evidence};
struct verification_dependency_node{std::uint64_t id=0;dependency_node_kind kind=dependency_node_kind::artifact;std::vector<std::uint8_t>key;bool diagnostic_only=false;};
struct verification_dependency_edge{std::uint64_t from=0,to=0;std::uint16_t relation=0;bool diagnostic_only=false;};
struct verification_dependency_graph{std::uint16_t schema=1;artifact_slot slot=artifact_slot::validated_operands;digest artifact_digest;std::vector<verification_dependency_node>nodes;std::vector<verification_dependency_edge>edges;};
struct dependency_slice{std::uint16_t schema=1;artifact_slot slot=artifact_slot::validated_operands;digest artifact_digest;std::vector<verification_dependency_node>nodes;std::vector<verification_dependency_edge>edges;digest slice_digest;};
struct canonical_graph_node{std::vector<std::uint8_t>initial_color;};
struct canonical_graph_arc{std::uint64_t from=0,to=0;std::uint16_t type=0;};
struct canonical_graph{std::vector<canonical_graph_node>nodes;std::vector<canonical_graph_arc>arcs;};
struct canonical_graph_result{std::vector<std::uint64_t>source_by_label;std::vector<std::uint8_t>canonical_bytes;};
struct verification_replay_archive{std::uint16_t schema=1;coordinate_tag coordinate=coordinate_tag::binary32;index_tag index=index_tag::uint32;digest setup_digest,artifact_digest,report_digest;std::vector<std::uint8_t>operand_a,operand_b,artifact,report;std::vector<digest>dependencies;digest archive_digest;};

class verifier_registry final:public verifier_service{
    struct key{artifact_slot slot;std::uint64_t type;bool operator<(const key&o)const noexcept{return slot!=o.slot?slot<o.slot:type<o.type;}};
    std::map<key,verifier_registration>entries_;bool frozen_=false;digest registry_digest_;
public:
    status_or<bool>register_verifier(verifier_registration);
    status_or<bool>freeze();
    bool frozen()const noexcept{return frozen_;}
    const digest&registry_digest()const noexcept{return registry_digest_;}
    status_or<verification_spec>specification(artifact_slot,std::uint64_t,std::uint16_t,verification_level)const;
    status_or<verification_report>verify(const artifact_view&,const verification_spec&,const verification_environment_view&)const noexcept override;
};

digest invariant_set_digest(const verification_spec&);
digest evidence_digest(const evidence_record&);
status_or<std::vector<std::uint8_t>>encode_evidence_record(const evidence_record&);
status_or<std::vector<std::uint8_t>>encode_verification_report(const verification_report&);
status_or<bool>validate_verification_report(const verification_report&);
status_or<dependency_slice>slice_dependencies(const verification_dependency_graph&,const std::vector<std::uint64_t>&,resource_accountant* = nullptr);
status_or<std::vector<std::uint8_t>>encode_dependency_slice(const dependency_slice&);
status_or<dependency_slice>decode_dependency_slice(const std::vector<std::uint8_t>&);
status_or<std::vector<std::uint8_t>>encode_replay_seed(const replay_seed&);
status_or<replay_seed>decode_replay_seed(const std::vector<std::uint8_t>&);
status_or<canonical_graph_result>canonicalize_graph_exhaustive(const canonical_graph&,resource_accountant*,const std::function<bool()>& = {});
status_or<std::vector<std::uint8_t>>encode_replay_archive(const verification_replay_archive&);
status_or<verification_replay_archive>decode_replay_archive(const std::vector<std::uint8_t>&,resource_accountant* = nullptr);

} }
#endif
