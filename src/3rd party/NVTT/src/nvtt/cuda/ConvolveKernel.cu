// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "CudaMath.h"

#define TW 16
#define TH 16

#define THREAD_COUNT 		(TW * TH)

#define MAX_KERNEL_WIDTH	32

#define KW 4



#if __DEVICE_EMULATION__
#define __debugsync() __syncthreads()
#else
#define __debugsync()
#endif

#define TN            256
#define WARP_COUNT    (TN / 32)
#define HWARP_COUNT   (TN / 16)

// Window size
#define WS            20



struct WrapClamp
{
	int operator()(int i, int h)
	{
		i = min(max(i, 0), h-1);
	}
};

struct WrapRepeat
{
	int operator()(int i, int h)
	{
		i = abs(i) % h;	// :( Non power of two!
	}
};

struct WrapMirror
{
	int operator()(int i, int h)
	{
		i = abs(i);
		while (i >= h) i = 2 * w - i - 2;
	}
};


// Vertical convolution filter that processes vertical strips.
__global__ void convolveStrip(float * d_channel, float * d_kernel, int width, int height)
{
	__shared__ float s_kernel[32 * WS];

	// Preload kernel in shared memory.
	for (int i = 0; i < 32 * WS / TN; i++)
	{
		int idx = i * TN + tid;
		if (idx < 32 * WS) s_kernel[idx] = d_kernel[idx];
	}

	__shared__ float s_strip[32 * WS];	// TN/32
	
	int wid = tid / 32 - WS/2;

	Mirror wrap;
	int row = wrap(wid);

	// Preload image block.
	for (int i = 0; i < 32 * WS / TN; i++)
	{
	}

	// @@ Apply kernel to TN/32 rows.

	// @@ Load 


}






__constant__ float inputGamma, outputInverseGamma;
__constant__ float kernel[MAX_KERNEL_WIDTH];

// Use texture to access input?
// That's the most simple approach.

texture<> image;

////////////////////////////////////////////////////////////////////////////////
// Combined convolution filter
////////////////////////////////////////////////////////////////////////////////

__global__ void convolve(float4 * output)
{
	// @@ Use morton order to assing threads.
	int x = threadIdx.x;
	int y = threadIdx.y;
	
	float4 color = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
	
	// texture coordinate.
	int2 t;
	t.x = 2 * (blockIdx.x * TW + x) - HW;
	t.y = blockIdx.y * TH + y;
	
	// @@ We might want to loop and process strips, to reuse the results of the horizontal convolutions.
	
	// Horizontal convolution. @@ Unroll loops.
	for (int e = HW; e > 0; e--)
	{
		t.x++;
		float w = kernel[e-1];
		color += w * tex2D(image, tc);
	}
	
	for (int e = 0; e < HW; e++)
	{
		t.x++;
		float w = kernel[e];
		color += w * tex2D(image, tc);
	}
	
	// Write color to shared memory.
	__shared__ float tile[4 * THREAD_COUNT];

	int tileIdx = y * TW + x;
	tile[tileIdx + 0 * THREAD_COUNT] = color.x;
	tile[tileIdx + 1 * THREAD_COUNT] = color.y;
	tile[tileIdx + 2 * THREAD_COUNT] = color.z;
	tile[tileIdx + 3 * THREAD_COUNT] = color.w;

	__syncthreads();
	
	// tile coordinate.
	t.x = x;
	t.y = y - HW;
	
	// Vertical convolution. @@ Unroll loops.
	for (int i = HW; i > 0; i--)
	{
		float w = kernel[i-1];
		
		t.y++;
		int idx = t.y * TW + t.x;
		
		color.x += w * tile[idx + 0 * THREAD_COUNT];
		color.y += w * tile[idx + 1 * THREAD_COUNT];
		color.z += w * tile[idx + 2 * THREAD_COUNT];
		color.w += w * tile[idx + 3 * THREAD_COUNT];
	}
	
	for (int i = 0; i < HW; i++)
	{
		float w = kernel[i];
		
		t.y++;
		int idx = t.y * TW + t.x;
		
		color.x += w * tile[idx + 0 * THREAD_COUNT];
		color.y += w * tile[idx + 1 * THREAD_COUNT];
		color.z += w * tile[idx + 2 * THREAD_COUNT];
		color.w += w * tile[idx + 3 * THREAD_COUNT];
	}	
	
	it (x < w && y < h)
	{
		// @@ Prevent unaligned writes.
		
		output[y * w + h] = color;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Monophase X convolution filter
////////////////////////////////////////////////////////////////////////////////

__device__ void convolveY()
{

}


////////////////////////////////////////////////////////////////////////////////
// Mipmap convolution filter
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// Gamma correction
////////////////////////////////////////////////////////////////////////////////

/*
__device__ float toLinear(float f, float gamma = 2.2f)
{
	return __pow(f, gamma);
}

__device__ float toGamma(float f, float gamma = 2.2f)
{
	return pow(f, 1.0f / gamma);
}
*/




////////////////////////////////////////////////////////////////////////////////
// Setup kernel
////////////////////////////////////////////////////////////////////////////////

extern "C" void setupConvolveKernel(const float * k, int w)
{
	w = min(w, MAX_KERNEL_WIDTH);
	cudaMemcpyToSymbol(kernel, k, sizeof(float) * w, 0);
}


////////////////////////////////////////////////////////////////////////////////
// Launch kernel
////////////////////////////////////////////////////////////////////////////////




