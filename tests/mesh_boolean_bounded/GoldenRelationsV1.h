#pragma once

// Committed golden known answers for the Component 07 canonical relation
// artifact.  The digest below is the SHA-256 of the complete canonical bytes
// published for the canonical golden fixture
//
//   operand A = unit box [0,1]^3
//   operand B = overlapping box [0.25,0.25,1.0] -> [0.75,0.75,2.0]
//   operation  = intersection
//   T/I        = double / uint32_t
//   triangulation provider = indexed_dependency_v1
//   execution  = serial_v1, single worker
//
// with the frozen Component 01-06 predecessor chain and strict C++17 floating
// environment.  Rebuilding this fixture with any future semantic change to the
// relation artifact must update this golden value as part of the reviewed
// version bump, never silently.
//
// The value is a placeholder until the canonical golden digest is captured from
// a verified build; TestRelationExactOracle.cc asserts against it and prints
// the observed digest when it disagrees.

#include <cstdint>

namespace ygor::mesh_boolean::qualification {

inline constexpr std::uint8_t golden_relation_artifact_digest_v1[32] = {
    0x6e, 0x9a, 0x7f, 0x29, 0xef, 0x43, 0xfd, 0x9b,
    0xad, 0xb2, 0x46, 0x32, 0x5d, 0x7c, 0x77, 0xf1,
    0xea, 0x94, 0xcf, 0xa3, 0xe1, 0x2f, 0xa2, 0x8c,
    0x01, 0x66, 0x86, 0x7c, 0xaf, 0x6a, 0xa9, 0xb6,
};

} // namespace ygor::mesh_boolean::qualification
