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

#ifndef NV_IMAGE_PIXELFORMAT_H
#define NV_IMAGE_PIXELFORMAT_H

#include <nvimage/nvimage.h>


namespace nv
{
	namespace PixelFormat
	{

		// Convert component @a c having @a inbits to the returned value having @a outbits.
		inline uint convert(uint c, uint inbits, uint outbits)
		{
			if (inbits == 0)
			{
				return 0;
			}
			else if (inbits >= outbits)
			{
				// truncate
				return c >> (inbits - outbits);
			}
			else
			{
				// bitexpand
				return (c << (outbits - inbits)) | convert(c, inbits, outbits - inbits);
			}
		}

		// Get pixel component shift and size given its mask.
		inline void maskShiftAndSize(uint mask, uint * shift, uint * size)
		{
			if (!mask)
			{
				*shift = 0;
				*size = 0;
				return;
			}

			*shift = 0;
			while((mask & 1) == 0) {
				++(*shift);
				mask >>= 1;
			}
			
			*size = 0;
			while((mask & 1) == 1) {
				++(*size);
				mask >>= 1;
			}
		}

	} // PixelFormat namespace

} // nv namespace


#endif // NV_IMAGE_PIXELFORMAT_H
