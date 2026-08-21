#include "YgorMeshesBooleanQualificationCorpusInternal.h"

#include <array>

namespace ygor {
namespace mesh_boolean {
namespace {
constexpr std::array<char,8> corpus_tag{{'Y','G','B','Q','B','C','0','1'}};
constexpr std::array<char,8> coverage_tag{{'Y','G','B','Q','B','V','0','1'}};
constexpr std::array<char,8> outcome_tag{{'Y','G','B','Q','B','O','0','1'}};
}

product_status_or<std::vector<qualification_corpus_binding>>
make_qualification_corpus_bindings(
    const qualification_corpus_inventory &inventory) noexcept {
  auto valid=validate_qualification_corpus_inventory(inventory);
  if(!valid.has_value()) return valid.error();
  try {
    std::vector<qualification_corpus_binding> result;
    result.reserve(inventory.records.size());
    for(const auto &record:inventory.records){
      qualification_corpus_binding b;
      b.identifier=inventory.identifier+"/"+record.identifier;
      b.version=inventory.version+"/"+record.recipe_version;
      b.source=record.source; b.redistribution=record.redistribution;
      b.license_or_provenance=record.license_or_provenance;
      b.case_count=record.case_count;
      canonical_encoder corpus;
      corpus.string(b.identifier); qualification_corpus_detail::encode_digest(corpus,inventory.inventory_digest);
      qualification_corpus_detail::encode_digest(corpus,record.record_digest);
      b.corpus_digest=domain_digest(corpus_tag,corpus.bytes());
      canonical_encoder coverage;
      coverage.string(record.identifier);
      for(auto v:record.geometry_categories)
        coverage.byte(static_cast<std::uint8_t>(v));
      for(const auto &v:record.operations){
        coverage.byte(static_cast<std::uint8_t>(v.selected_operation));
        coverage.byte(static_cast<std::uint8_t>(v.operand_order));
      }
      for(const auto &v:record.type_specializations){
        coverage.byte(static_cast<std::uint8_t>(v.coordinate));
        coverage.byte(static_cast<std::uint8_t>(v.index));
      }
      for(const auto &v:record.result_modes){
        coverage.byte(static_cast<std::uint8_t>(v.representation));
        coverage.byte(static_cast<std::uint8_t>(v.semantics));
      }
      for(const auto &v:record.preparation_policies)
        coverage.byte(static_cast<std::uint8_t>(v.mode));
      b.category_coverage_digest=domain_digest(coverage_tag,coverage.bytes());
      canonical_encoder outcomes;
      outcomes.string(record.expectation_identifier);
      qualification_corpus_detail::encode_digest(outcomes,record.expectation_digest);
      for(auto v:record.expected_outcomes)
        outcomes.byte(static_cast<std::uint8_t>(v));
      for(auto v:record.expected_failure_codes)
        outcomes.u16(static_cast<std::uint16_t>(v));
      b.expected_outcome_digest=domain_digest(outcome_tag,outcomes.bytes());
      result.push_back(std::move(b));
    }
    return result;
  }catch(...){return qualification_corpus_detail::error("qualification_corpus_binding_exception");}
}

} // namespace mesh_boolean
} // namespace ygor
