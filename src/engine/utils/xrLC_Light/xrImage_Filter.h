#pragma once

void imf_BuildKernel	(float* dest, float* src, int DIM, float norm=1.f);
void imf_ProcessKernel	(Fvector* dest, Fvector* src, int w, int h, float* kern, int DIM);

// some filters
extern float imk_blur_gauss [7][7];
