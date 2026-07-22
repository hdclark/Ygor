#pragma once

#ifndef YGOR_MESH_BOOLEAN_STRICT_FP_BUILD
#error "Bounded mesh Boolean sources require the strict floating-point target"
#endif
#ifdef __FAST_MATH__
#error "Fast-math is incompatible with bounded mesh Boolean contracts"
#endif
