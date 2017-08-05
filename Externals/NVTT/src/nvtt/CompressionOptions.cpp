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

#include "nvtt.h"
#include "CompressionOptions.h"

using namespace nv;
using namespace nvtt;


/// Constructor. Sets compression options to the default values.
CompressionOptions::CompressionOptions() : m(*new CompressionOptions::Private())
{
	reset();
}


/// Destructor.
CompressionOptions::~CompressionOptions()
{
	delete &m;
}


/// Set default compression options.
void CompressionOptions::reset()
{
	m.format = Format_DXT1;
	m.quality = Quality_Normal;
	m.colorWeight.set(1.0f, 1.0f, 1.0f, 1.0f);

	m.bitcount = 32;
	m.bmask = 0x000000FF;
	m.gmask = 0x0000FF00;
	m.rmask = 0x00FF0000;
	m.amask = 0xFF000000;

	m.enableColorDithering = false;
	m.enableAlphaDithering = false;
	m.binaryAlpha = false;
	m.alphaThreshold = 127;
}


/// Set desired compression format.
void CompressionOptions::setFormat(Format format)
{
	m.format = format;
}


/// Set compression quality settings.
void CompressionOptions::setQuality(Quality quality)
{
	m.quality = quality;
}


/// Set the weights of each color channel. 
/// The choice for these values is subjective. In many case uniform color weights 
/// (1.0, 1.0, 1.0) work very well. A popular choice is to use the NTSC luma encoding 
/// weights (0.2126, 0.7152, 0.0722), but I think that blue contributes to our 
/// perception more than a 7%. A better choice in my opinion is (3, 4, 2).
void CompressionOptions::setColorWeights(float red, float green, float blue, float alpha/*=1.0f*/)
{
//	float total = red + green + blue;
//	float x = red / total;
//	float y = green / total;
//	m.colorWeight.set(x, y, 1.0f - x - y);
	m.colorWeight.set(red, green, blue, alpha);
}


/// Set color mask to describe the RGB/RGBA format.
void CompressionOptions::setPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask)
{
	// Validate arguments.
	nvCheck(bitcount == 8 || bitcount == 16 || bitcount == 24 || bitcount == 32);
	nvCheck((rmask & gmask) == 0);
	nvCheck((rmask & bmask) == 0);
	nvCheck((rmask & amask) == 0);
	nvCheck((gmask & bmask) == 0);
	nvCheck((gmask & amask) == 0);
	nvCheck((bmask & amask) == 0);

	if (bitcount != 32)
	{
		uint maxMask = (1 << bitcount);
		nvCheck(maxMask > rmask);
		nvCheck(maxMask > gmask);
		nvCheck(maxMask > bmask);
		nvCheck(maxMask > amask);
	}

	m.bitcount = bitcount;
	m.rmask = rmask;
	m.gmask = gmask;
	m.bmask = bmask;
	m.amask = amask;
}

/// Use external compressor.
void CompressionOptions::setExternalCompressor(const char * name)
{
	m.externalCompressor = name;
}

/// Set quantization options.
/// @warning Do not enable dithering unless you know what you are doing. Quantization 
/// introduces errors. It's better to let the compressor quantize the result to 
/// minimize the error, instead of quantizing the data before handling it to
/// the compressor.
void CompressionOptions::setQuantization(bool colorDithering, bool alphaDithering, bool binaryAlpha, int alphaThreshold/*= 127*/)
{
	nvCheck(alphaThreshold >= 0 && alphaThreshold < 256);
	m.enableColorDithering = colorDithering;
	m.enableAlphaDithering = alphaDithering;
	m.binaryAlpha = binaryAlpha;
	m.alphaThreshold = alphaThreshold;
}



