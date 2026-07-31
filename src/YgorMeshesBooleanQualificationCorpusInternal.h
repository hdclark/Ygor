#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_CORPUS_INTERNAL_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_CORPUS_INTERNAL_H_

#include "YgorMeshesBooleanQualificationCorpus.h"

namespace ygor {
namespace mesh_boolean {
namespace qualification_corpus_detail {

product_error error(const char *);
bool digest_zero(const digest &) noexcept;
void encode_digest(canonical_encoder &, const digest &);
product_status_or<qualification_corpus_record>
canonicalize_record(qualification_corpus_record);

} // namespace qualification_corpus_detail
} // namespace mesh_boolean
} // namespace ygor

#endif
