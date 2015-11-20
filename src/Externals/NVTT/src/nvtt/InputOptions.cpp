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

#include <string.h> // memcpy

#include <nvcore/Memory.h>

#include "nvtt.h"
#include "InputOptions.h"

using namespace nv;
using namespace nvtt;

namespace
{

	static uint countMipmaps(int w, int h, int d)
	{
		uint mipmap = 0;
		
		while (w != 1 || h != 1 || d != 1) {
			w = max(1, w / 2);
			h = max(1, h / 2);
			d = max(1, d / 2);
			mipmap++;
		}
		
		return mipmap + 1;
	}

	// 1 -> 1, 2 -> 2, 3 -> 2, 4 -> 4, 5 -> 4, ...
	static uint previousPowerOfTwo(const uint v)
	{
		return nextPowerOfTwo(v + 1) / 2;
	}
	
	static uint nearestPowerOfTwo(const uint v)
	{
		const uint np2 = nextPowerOfTwo(v);
		const uint pp2 = previousPowerOfTwo(v);
		
		if (np2 - v <= v - pp2)
		{
			return np2;
		}
		else
		{
			return pp2;
		}
	}
	
} // namespace


/// Constructor.
InputOptions::InputOptions() : m(*new InputOptions::Private())
{ 
	reset();
}

// Delete images.
InputOptions::~InputOptions()
{
	resetTextureLayout();
	
	delete &m;
}


// Reset input options.
void InputOptions::reset()
{
	m.wrapMode = WrapMode_Mirror;
	m.textureType = TextureType_2D;
	m.inputFormat = InputFormat_BGRA_8UB;

	m.alphaMode = AlphaMode_None;

	m.inputGamma = 2.2f;
	m.outputGamma = 2.2f;
	
	m.colorTransform = ColorTransform_None;
	m.linearTransform = Matrix(identity);

	m.generateMipmaps = true;
	m.maxLevel = -1;
	m.mipmapFilter = MipmapFilter_Box;

	m.kaiserWidth = 3;
	m.kaiserAlpha = 4.0f;
	m.kaiserStretch = 1.0f;

	m.isNormalMap = false;
	m.normalizeMipmaps = true;
	m.convertToNormalMap = false;
	m.heightFactors.set(0.0f, 0.0f, 0.0f, 1.0f);
	m.bumpFrequencyScale = Vector4(1.0f, 0.5f, 0.25f, 0.125f) / (1.0f + 0.5f + 0.25f + 0.125f);
	
	m.maxExtent = 0;
	m.roundMode = RoundMode_None;
}


// Setup the input image.
void InputOptions::setTextureLayout(TextureType type, int width, int height, int depth /*= 1*/)
{
	// Validate arguments.
	nvCheck(width >= 0);
	nvCheck(height >= 0);
	nvCheck(depth >= 0);

	// Correct arguments.
	if (width == 0) width = 1;
	if (height == 0) height = 1;
	if (depth == 0) depth = 1;

	// Delete previous images.
	resetTextureLayout();
	
	m.textureType = type;
	
	// Allocate images.
	m.mipmapCount = countMipmaps(width, height, depth);
	m.faceCount = (type == TextureType_Cube) ? 6 : 1;
	m.imageCount = m.mipmapCount * m.faceCount;
	
	m.images = new Private::InputImage[m.imageCount];
	
	for(uint f = 0; f < m.faceCount; f++)
	{
		uint w = width;
		uint h = height;
		uint d = depth;

		for (uint mipLevel = 0; mipLevel < m.mipmapCount; mipLevel++)
		{
			Private::InputImage & img = m.images[f * m.mipmapCount + mipLevel];
			img.width = w;
			img.height = h;
			img.depth = d;
			img.mipLevel = mipLevel;
			img.face = f;
			
			img.data = NULL;
			
			w = max(1U, w / 2);
			h = max(1U, h / 2);
			d = max(1U, d / 2);
		}
	}
}


void InputOptions::resetTextureLayout()
{
	if (m.images != NULL)
	{
		// Delete image array.
		delete [] m.images;
		m.images = NULL;

		m.faceCount = 0;
		m.mipmapCount = 0;
		m.imageCount = 0;
	}
}


// Copies the data to our internal structures.
bool InputOptions::setMipmapData(const void * data, int width, int height, int depth /*= 1*/, int face /*= 0*/, int mipLevel /*= 0*/)
{
	nvCheck(depth == 1);
	
	const int idx = face * m.mipmapCount + mipLevel;
	
	if (m.images[idx].width != width || m.images[idx].height != height || m.images[idx].depth != depth || m.images[idx].mipLevel != mipLevel || m.images[idx].face != face)
	{
		// Invalid dimension or index.
		return false;
	}
	
	m.images[idx].data = new nv::Image();
	m.images[idx].data->allocate(width, height);
	memcpy(m.images[idx].data->pixels(), data, width * height * 4); 
	
	return true;
}


/// Describe the format of the input.
void InputOptions::setFormat(InputFormat format)
{
	m.inputFormat = format;
}


/// Set the way the input alpha channel is interpreted.
void InputOptions::setAlphaMode(AlphaMode alphaMode)
{
	m.alphaMode = alphaMode;
}


/// Set gamma settings.
void InputOptions::setGamma(float inputGamma, float outputGamma)
{
	m.inputGamma = inputGamma;
	m.outputGamma = outputGamma;
}


/// Set texture wrappign mode.
void InputOptions::setWrapMode(WrapMode mode)
{
	m.wrapMode = mode;
}


/// Set mipmap filter.
void InputOptions::setMipmapFilter(MipmapFilter filter)
{
	m.mipmapFilter = filter;
}

/// Set mipmap generation.
void InputOptions::setMipmapGeneration(bool enabled, int maxLevel/*= -1*/)
{
	m.generateMipmaps = enabled;
	m.maxLevel = maxLevel;
}

/// Set Kaiser filter parameters.
void InputOptions::setKaiserParameters(float width, float alpha, float stretch)
{
	m.kaiserWidth = width;
	m.kaiserAlpha = alpha;
	m.kaiserStretch = stretch;
}

/// Indicate whether input is a normal map or not.
void InputOptions::setNormalMap(bool b)
{
	m.isNormalMap = b;
}

/// Enable normal map conversion.
void InputOptions::setConvertToNormalMap(bool convert)
{
	m.convertToNormalMap = convert;
}

/// Set height evaluation factors.
void InputOptions::setHeightEvaluation(float redScale, float greenScale, float blueScale, float alphaScale)
{
	// Do not normalize height factors.
//	float total = redScale + greenScale + blueScale + alphaScale;
	m.heightFactors = Vector4(redScale, greenScale, blueScale, alphaScale);
}

/// Set normal map conversion filter.
void InputOptions::setNormalFilter(float small, float medium, float big, float large)
{
	float total = small + medium + big + large;
	m.bumpFrequencyScale = Vector4(small, medium, big, large) / total;
}

/// Enable mipmap normalization.
void InputOptions::setNormalizeMipmaps(bool normalize)
{
	m.normalizeMipmaps = normalize;
}

/// Set color transform.
void InputOptions::setColorTransform(ColorTransform t)
{
	m.colorTransform = t;
}

// Set linear transform for the given channel.
void InputOptions::setLinearTransform(int channel, float w0, float w1, float w2, float w3)
{
	nvCheck(channel >= 0 && channel < 4);

	Vector4 w(w0, w1, w2, w3);
	//m.linearTransform.setRow(channel, w);
}

void InputOptions::setMaxExtents(int e)
{
	nvDebugCheck(e > 0);
	m.maxExtent = e;
}

void InputOptions::setRoundMode(RoundMode mode)
{
	m.roundMode = mode;
}


void InputOptions::Private::computeTargetExtents() const
{
	nvCheck(images != NULL);
	
	uint maxExtent = this->maxExtent;
	if (roundMode != RoundMode_None)
	{
		// rounded max extent should never be higher than original max extent.
		maxExtent = previousPowerOfTwo(maxExtent);
	}

	uint w = images->width;
	uint h = images->height;
	uint d = images->depth;
	
	nvDebugCheck(w > 0);
	nvDebugCheck(h > 0);
	nvDebugCheck(d > 0);
	
	// Scale extents without changing aspect ratio.
	uint maxwhd = max(max(w, h), d);
	if (maxExtent != 0 && maxwhd > maxExtent)
	{
		w = max((w * maxExtent) / maxwhd, 1U);
		h = max((h * maxExtent) / maxwhd, 1U);
		d = max((d * maxExtent) / maxwhd, 1U);
	}
	
	// Round to power of two.
	if (roundMode == RoundMode_ToNextPowerOfTwo)
	{
		w = nextPowerOfTwo(w);
		h = nextPowerOfTwo(h);
		d = nextPowerOfTwo(d);
	}
	else if (roundMode == RoundMode_ToNearestPowerOfTwo)
	{
		w = nearestPowerOfTwo(w);
		h = nearestPowerOfTwo(h);
		d = nearestPowerOfTwo(d);
	}
	else if (roundMode == RoundMode_ToPreviousPowerOfTwo)
	{
		w = previousPowerOfTwo(w);
		h = previousPowerOfTwo(h);
		d = previousPowerOfTwo(d);
	}
	
	this->targetWidth = w;
	this->targetHeight = h;
	this->targetDepth = d;
	
	this->targetMipmapCount = countMipmaps(w, h, d);
}


// Return real number of mipmaps, including first level.
// computeTargetExtents should have been called before.
int InputOptions::Private::realMipmapCount() const
{
	int mipmapCount = targetMipmapCount;
	
	if (!generateMipmaps) mipmapCount = 1;
	else if (maxLevel != -1 && maxLevel < mipmapCount - 1) mipmapCount = maxLevel + 1;

	return mipmapCount;
}


const Image * InputOptions::Private::image(uint face, uint mipmap) const
{
	nvDebugCheck(face < faceCount);
	nvDebugCheck(mipmap < mipmapCount);

	const InputImage & image = this->images[face * mipmapCount + mipmap];
	nvDebugCheck(image.face == face);
	nvDebugCheck(image.mipLevel == mipmap);

	return image.data.ptr();
}

const Image * InputOptions::Private::image(uint idx) const
{
	nvDebugCheck(idx < faceCount * mipmapCount);

	const InputImage & image = this->images[idx];

	return image.data.ptr();
}
