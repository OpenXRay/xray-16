#pragma once

#include "psystem.h"

void noise3Init();

namespace PAPI
{
PARTICLES_API void GetNoise3Tables(int* permutation, float* gradients);
}

float noise3(const Fvector& vec);
float fractalsum3(const Fvector& v, float freq, int octaves);
float turbulence3(const Fvector& v, float freq, int octaves);
