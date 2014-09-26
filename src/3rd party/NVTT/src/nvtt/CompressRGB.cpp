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

#include <nvcore/Debug.h>

#include <nvimage/Image.h>
#include <nvimage/PixelFormat.h>
#include <nvmath/Color.h>

#include "CompressRGB.h"
#include "CompressionOptions.h"
#include "OutputOptions.h"

using namespace nv;
using namespace nvtt;

namespace 
{

	inline uint computePitch(uint w, uint bitsize)
	{
		uint p = w * ((bitsize + 7) / 8);

		// Align to 32 bits.
		return ((p + 3) / 4) * 4;
	}

	inline void convert_to_a8r8g8b8(const void * src, void * dst, uint w)
	{
		memcpy(dst, src, 4 * w);
	}

	inline void convert_to_x8r8g8b8(const void * src, void * dst, uint w)
	{
		memcpy(dst, src, 4 * w);
	}

} // namespace


// Pixel format converter.
void nv::compressRGB(const Image * image, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	nvCheck(image != NULL);

	const uint w = image->width();
	const uint h = image->height();

	const uint bitCount = compressionOptions.bitcount;
	nvCheck(bitCount == 8 || bitCount == 16 || bitCount == 24 || bitCount == 32);

	const uint byteCount = bitCount / 8;

	const uint rmask = compressionOptions.rmask;
	uint rshift, rsize;
	PixelFormat::maskShiftAndSize(rmask, &rshift, &rsize);
	
	const uint gmask = compressionOptions.gmask;
	uint gshift, gsize;
	PixelFormat::maskShiftAndSize(gmask, &gshift, &gsize);
	
	const uint bmask = compressionOptions.bmask;
	uint bshift, bsize;
	PixelFormat::maskShiftAndSize(bmask, &bshift, &bsize);
	
	const uint amask = compressionOptions.amask;
	uint ashift, asize;
	PixelFormat::maskShiftAndSize(amask, &ashift, &asize);

	// Determine pitch.
	uint pitch = computePitch(w, compressionOptions.bitcount);

	uint8 * dst = (uint8 *)mem::malloc(pitch + 4);

	for (uint y = 0; y < h; y++)
	{
		const Color32 * src = image->scanline(y);

		if (bitCount == 32 && rmask == 0xFF0000 && gmask == 0xFF00 && bmask == 0xFF && amask == 0xFF000000)
		{
			convert_to_a8r8g8b8(src, dst, w);
		}
		else if (bitCount == 32 && rmask == 0xFF0000 && gmask == 0xFF00 && bmask == 0xFF && amask == 0)
		{
			convert_to_x8r8g8b8(src, dst, w);
		}
		else
		{
			// Generic pixel format conversion.
			for (uint x = 0; x < w; x++)
			{
				uint c = 0;
				c |= PixelFormat::convert(src[x].r, 8, rsize) << rshift;
				c |= PixelFormat::convert(src[x].g, 8, gsize) << gshift;
				c |= PixelFormat::convert(src[x].b, 8, bsize) << bshift;
				c |= PixelFormat::convert(src[x].a, 8, asize) << ashift;
				
				// Output one byte at a time.
				for (uint i = 0; i < byteCount; i++)
				{
					*(dst + x * byteCount + i) = (c >> (i * 8)) & 0xFF;
				}
			}
			
			// Zero padding.
			for (uint x = w * byteCount; x < pitch; x++)
			{
				*(dst + x) = 0;
			}
		}

		if (outputOptions.outputHandler != NULL)
		{
			outputOptions.outputHandler->writeData(dst, pitch);
		}
	}

	mem::free(dst);
}

