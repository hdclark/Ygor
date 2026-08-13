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
    0xfc, 0x90, 0xdc, 0x0b, 0xa5, 0xee, 0xcb, 0x0b,
    0xa0, 0x95, 0xe3, 0x75, 0x32, 0x0b, 0x8e, 0x79,
    0xa1, 0x9a, 0x3f, 0xf3, 0x1a, 0x9c, 0xf5, 0x9a,
    0x7f, 0x1c, 0xd1, 0x97, 0x2a, 0x6a, 0x80, 0x03,
};

} // namespace ygor::mesh_boolean::qualification
