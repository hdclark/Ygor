#pragma once
#include "Context.h"
namespace ygor::mesh_boolean::bounded { template<class T,class I>bool verify_context(const boolean_context<T,I>&c){return c.sources&&verify_source(c.sources->a)&&verify_source(c.sources->b)&&materialize_truth_table().digest==c.truth.digest&&verify_symbolic_policy(c.symbolic)&&c.sources->digest==c.input_digest&&c.precision.source_digest==c.input_digest&&compute_context_semantic_digest(c)==c.context_digest;} }
