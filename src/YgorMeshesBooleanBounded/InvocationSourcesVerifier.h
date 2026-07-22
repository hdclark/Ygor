#pragma once
#include "InvocationSources.h"
namespace ygor::mesh_boolean::bounded { template<class T,class I>bool verify_invocation_sources(const immutable_invocation_sources<T,I>&s){sha256 h;h.update(s.a.canonical_bytes());h.update(s.b.canonical_bytes());return verify_source(s.a)&&verify_source(s.b)&&h.finish()==s.digest;} }
