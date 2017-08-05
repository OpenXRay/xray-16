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

#include <nvcore/Ptr.h>

#include <nvmath/Color.h>

#include <nvimage/NormalMap.h>
#include <nvimage/Filter.h>
#include <nvimage/FloatImage.h>
#include <nvimage/Image.h>

using namespace nv;


static float processPixel(const FloatImage * img, uint x, uint y)
{
	nvDebugCheck(img != NULL);
	
	const uint w = img->width();
	const uint h = img->height();
	
	float d = img->pixel(x, y, 0);
	
	float fx0 = (float) x / w;
	float fy0 = (float) y / h;

	float best_ratio = INFINITY;
	uint best_x = w;
	uint best_y = h;
	
	for (uint yy = 0; yy < h; yy++)
	{
		for (uint xx = 0; xx < w; xx++)
		{
			float ch = d - img->pixel(xx, yy, 0);
			
			if (ch > 0)
			{
				float dx = float(xx - x);
				float dy = float(yy - y);
				
				float ratio = (dx * dx + dy * dy) / ch;
				
				if (ratio < best_ratio)
				{
					best_x = xx;
					best_y = yy;
				}
			}
		}
	}

	if (best_x != w)
	{
		nvDebugCheck(best_y !=h);
		
		float dx = float(best_x - x) / w;
		float dy = float(best_y - y) / h;
		
		float cw = sqrtf(dx*dx + dy*dy);
		float ch = d - img->pixel(best_x, best_y, 0);
		
		return min(1.0f, sqrtf(cw / ch));
	}
	
	return 1.0f;
}


// Create cone map using the given kernels.
FloatImage * createConeMap(const Image * img, Vector4::Arg heightWeights)
{
	nvCheck(img != NULL);
	
	const uint w = img->width();
	const uint h = img->height();
	
	AutoPtr<FloatImage> fimage(new FloatImage());
	//fimage->allocate(2, w, h);
	fimage->allocate(4, w, h);
	
	// Compute height and store in red channel:
	float * heightChannel = fimage->channel(0);
	for(uint i = 0; i < w*h; i++)
	{
		Vector4 color = toVector4(img->pixel(i));
		heightChannel[i] = dot(color, heightWeights);
	}
	
	// Compute cones:
	for(uint y = 0; y < h; y++)
	{
		for(uint x = 0; x < w; x++)
		{
			processPixel(fimage.ptr(), x, y);
		}
	}
	
	return fimage.release();
}

