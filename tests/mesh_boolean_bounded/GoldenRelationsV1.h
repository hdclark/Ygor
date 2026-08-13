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
    0x0f, 0xc6, 0x9c, 0x3f, 0x91, 0x2c, 0x8c, 0x6f,
    0xaa, 0x2b, 0xb1, 0x85, 0x5e, 0xa2, 0x96, 0x10,
    0xb5, 0x8a, 0x00, 0x47, 0x98, 0xf3, 0xe1, 0xd0,
    0x32, 0x8c, 0x57, 0xbe, 0xcb, 0xe5, 0x4f, 0x5e,
};

} // namespace ygor::mesh_boolean::qualification
