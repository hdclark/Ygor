//YgorNoise.h

#ifndef YGOR_NOISE_HDR_GRD_H
#define YGOR_NOISE_HDR_GRD_H

#include "YgorDefinitions.h"

//--------------------------------------------------------------------------------------------------------
//----------------------------------------- Simple Perlin Noise. -----------------------------------------
//--------------------------------------------------------------------------------------------------------
float Perlin_Noise_3D(float vec[3]);


//--------------------------------------------------------------------------------------------------------
//------------------------------------ Generic, Integer, Non-Smooth Noise --------------------------------
//--------------------------------------------------------------------------------------------------------
//These are not (intentionally?) coherent noise generators, but they are consistent. They could be used to
// identify agents to one another, given a random input. They are obviously not *good* for this purpose, 
// but are better than a static keyword-based authentication.
uint64_t Number_Jumbler_A(uint64_t x);



#endif
