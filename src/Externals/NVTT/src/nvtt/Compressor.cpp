// Copyright NVIDIA Corporation 2008 -- Ignacio Castano <icastano@nvidia.com>
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

#include <nvtt/nvtt.h>

#include <nvcore/Memory.h>
#include <nvcore/Ptr.h>

#include <nvimage/DirectDrawSurface.h>
#include <nvimage/ColorBlock.h>
#include <nvimage/BlockDXT.h>
#include <nvimage/Image.h>
#include <nvimage/FloatImage.h>
#include <nvimage/Filter.h>
#include <nvimage/Quantize.h>
#include <nvimage/NormalMap.h>
#include <nvimage/PixelFormat.h>

#include "Compressor.h"
#include "InputOptions.h"
#include "CompressionOptions.h"
#include "OutputOptions.h"

#include "CompressDXT.h"
#include "CompressRGB.h"
#include "cuda/CudaUtils.h"
#include "cuda/CudaCompressDXT.h"


using namespace nv;
using namespace nvtt;


namespace
{

	static int blockSize(Format format)
	{
		if (format == Format_DXT1 || format == Format_DXT1a) {
			return 8;
		}
		else if (format == Format_DXT3) {
			return 16;
		}
		else if (format == Format_DXT5 || format == Format_DXT5n) {
			return 16;
		}
		else if (format == Format_BC4) {
			return 8;
		}
		else if (format == Format_BC5) {
			return 16;
		}
		return 0;
	}

	inline uint computePitch(uint w, uint bitsize)
	{
		uint p = w * ((bitsize + 7) / 8);

		// Align to 32 bits.
		return ((p + 3) / 4) * 4;
	}

	static int computeImageSize(uint w, uint h, uint d, uint bitCount, Format format)
	{
		if (format == Format_RGBA) {
			return d * h * computePitch(w, bitCount);
		}
		else {
			// @@ Handle 3D textures. DXT and VTC have different behaviors.
			return ((w + 3) / 4) * ((h + 3) / 4) * blockSize(format);
		}
	}

} // namespace

namespace nvtt
{
	// Mipmap could be:
	// - a pointer to an input image.
	// - a fixed point image.
	// - a floating point image.
	struct Mipmap
	{
		Mipmap() : m_inputImage(NULL) {}
		~Mipmap() {}

		// Reference input image.
		void setFromInput(const InputOptions::Private & inputOptions, uint idx)
		{
			m_inputImage = inputOptions.image(idx);
			m_fixedImage = NULL;
			m_floatImage = NULL;
		}

		// Assign and take ownership of given image.
		void setImage(FloatImage * image)
		{
			m_inputImage = NULL;
			m_fixedImage = NULL;
			m_floatImage = image;
		}


		// Convert linear float image to fixed image ready for compression.
		void toFixedImage(const InputOptions::Private & inputOptions)
		{
			if (m_floatImage != NULL) // apfaffe - We should check that we have a float image, if so convert it!
			{
				if (inputOptions.isNormalMap || inputOptions.outputGamma == 1.0f)
				{
					m_fixedImage = m_floatImage->createImage();
				}
				else
				{
					m_fixedImage = m_floatImage->createImageGammaCorrect(inputOptions.outputGamma);
				}
			}
		}

		// Convert input image to linear float image.
		void toFloatImage(const InputOptions::Private & inputOptions)
		{
			if (m_floatImage == NULL)
			{
				nvDebugCheck(this->asFixedImage() != NULL);

				m_floatImage = new FloatImage(this->asFixedImage());

				if (inputOptions.isNormalMap)
				{
					// Expand normals to [-1, 1] range.
					//	floatImage->expandNormals(0);
				}
				else if (inputOptions.inputGamma != 1.0f)
				{
					// Convert to linear space.
					m_floatImage->toLinear(0, 3, inputOptions.inputGamma);
				}
			}
		}

		const FloatImage * asFloatImage() const
		{
			return m_floatImage.ptr();
		}

		FloatImage * asFloatImage()
		{
			return m_floatImage.ptr();
		}

		const Image * asFixedImage() const
		{
			// - apfaffe - switched logic to return the 'processed image' rather than the input!
			if (m_fixedImage != NULL && m_fixedImage.ptr() != NULL)
			{
				return m_fixedImage.ptr();
			}
			return m_inputImage;
		}

		Image * asMutableFixedImage()
		{
			if (m_inputImage != NULL)
			{
				// Do not modify input image, create a copy.
				m_fixedImage = new Image(*m_inputImage);
				m_inputImage = NULL;
			}
			return m_fixedImage.ptr();
		}


	private:
		const Image * m_inputImage;
		AutoPtr<Image> m_fixedImage;
		AutoPtr<FloatImage> m_floatImage;
	};

} // nvtt namespace


Compressor::Compressor() : m(*new Compressor::Private())
{
	// CUDA initialization.
	m.cudaSupported = cuda::isHardwarePresent();
	m.cudaEnabled = false;
	m.cudaDevice = -1;

	enableCudaAcceleration(m.cudaSupported);
}

Compressor::~Compressor()
{
	enableCudaAcceleration(false);
	delete &m;
}


/// Enable CUDA acceleration.
void Compressor::enableCudaAcceleration(bool enable)
{
	if (m.cudaSupported)
	{
		if (m.cudaEnabled && !enable)
		{
			m.cudaEnabled = false;
			m.cuda = NULL;

			if (m.cudaDevice != -1)
			{
				// Exit device.
				cuda::exitDevice();
			}
		}
		else if (!m.cudaEnabled && enable)
		{
			// Init the CUDA device. This may return -1 if CUDA was already initialized by the app.
			m.cudaEnabled = cuda::initDevice(&m.cudaDevice);

			if (m.cudaEnabled)
			{
				// Create compressor if initialization succeeds.
				m.cuda = new CudaCompressor();

				// But cleanup if failed.
				if (!m.cuda->isValid())
				{
					enableCudaAcceleration(false);
				}
			}
		}
	}
}

/// Check if CUDA acceleration is enabled.
bool Compressor::isCudaAccelerationEnabled() const
{
	return m.cudaEnabled;
}


/// Compress the input texture with the given compression options.
bool Compressor::process(const InputOptions & inputOptions, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const
{
	return m.compress(inputOptions.m, compressionOptions.m, outputOptions.m);
}


/// Estimate the size of compressing the input with the given options.
int Compressor::estimateSize(const InputOptions & inputOptions, const CompressionOptions & compressionOptions) const
{
	return m.estimateSize(inputOptions.m, compressionOptions.m);
}




bool Compressor::Private::compress(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const
{
	// Make sure enums match.
	nvStaticCheck(FloatImage::WrapMode_Clamp == (FloatImage::WrapMode)WrapMode_Clamp);
	nvStaticCheck(FloatImage::WrapMode_Mirror == (FloatImage::WrapMode)WrapMode_Mirror);
	nvStaticCheck(FloatImage::WrapMode_Repeat == (FloatImage::WrapMode)WrapMode_Repeat);

	// Get output handler.
	if (!outputOptions.openFile())
	{
		if (outputOptions.errorHandler) outputOptions.errorHandler->error(Error_FileOpen);
		return false;
	}

	inputOptions.computeTargetExtents();

	// Output DDS header.
	if (!outputHeader(inputOptions, compressionOptions, outputOptions))
	{
		return false;
	}

	for (uint f = 0; f < inputOptions.faceCount; f++)
	{
		if (!compressMipmaps(f, inputOptions, compressionOptions, outputOptions))
		{
			return false;
		}
	}

	outputOptions.closeFile();

	return true;
}


// Output DDS header.
bool Compressor::Private::outputHeader(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const
{
	// Output DDS header.
	if (outputOptions.outputHandler == NULL || !outputOptions.outputHeader)
	{
		return true;
	}

	DDSHeader header;

	header.setWidth(inputOptions.targetWidth);
	header.setHeight(inputOptions.targetHeight);

	int mipmapCount = inputOptions.realMipmapCount();
	nvDebugCheck(mipmapCount > 0);

	header.setMipmapCount(mipmapCount);

	if (inputOptions.textureType == TextureType_2D) {
		header.setTexture2D();
	}
	else if (inputOptions.textureType == TextureType_Cube) {
		header.setTextureCube();
	}		
	/*else if (inputOptions.textureType == TextureType_3D) {
	header.setTexture3D();
	header.setDepth(inputOptions.targetDepth);
	}*/

	if (compressionOptions.format == Format_RGBA)
	{
		header.setPitch(computePitch(inputOptions.targetWidth, compressionOptions.bitcount));
		header.setPixelFormat(compressionOptions.bitcount, compressionOptions.rmask, compressionOptions.gmask, compressionOptions.bmask, compressionOptions.amask);
	}
	else
	{
		header.setLinearSize(computeImageSize(inputOptions.targetWidth, inputOptions.targetHeight, inputOptions.targetDepth, compressionOptions.bitcount, compressionOptions.format));

		if (compressionOptions.format == Format_DXT1 || compressionOptions.format == Format_DXT1a) {
			header.setFourCC('D', 'X', 'T', '1');
			if (inputOptions.isNormalMap) header.setNormalFlag(true);
		}
		else if (compressionOptions.format == Format_DXT3) {
			header.setFourCC('D', 'X', 'T', '3');
		}
		else if (compressionOptions.format == Format_DXT5) {
			header.setFourCC('D', 'X', 'T', '5');
		}
		else if (compressionOptions.format == Format_DXT5n) {
			header.setFourCC('D', 'X', 'T', '5');
			if (inputOptions.isNormalMap) header.setNormalFlag(true);
		}
		else if (compressionOptions.format == Format_BC4) {
			header.setFourCC('A', 'T', 'I', '1');
		}
		else if (compressionOptions.format == Format_BC5) {
			header.setFourCC('A', 'T', 'I', '2');
			if (inputOptions.isNormalMap) header.setNormalFlag(true);
		}
	}

	// Swap bytes if necessary.
	header.swapBytes();

	uint headerSize = 128;
	if (header.hasDX10Header())
	{
		nvStaticCheck(sizeof(DDSHeader) == 128 + 20);
		headerSize = 128 + 20;
	}

	bool writeSucceed = outputOptions.outputHandler->writeData(&header, headerSize);
	if (!writeSucceed && outputOptions.errorHandler != NULL)
	{
		outputOptions.errorHandler->error(Error_FileWrite);
	}

	return writeSucceed;
}


bool Compressor::Private::compressMipmaps(uint f, const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const
{
	uint w = inputOptions.targetWidth;
	uint h = inputOptions.targetHeight;
	uint d = inputOptions.targetDepth;

	Mipmap mipmap;

	const uint mipmapCount = inputOptions.realMipmapCount();
	nvDebugCheck(mipmapCount > 0);

	for (uint m = 0; m < mipmapCount; m++)
	{
		if (outputOptions.outputHandler)
		{
			int size = computeImageSize(w, h, d, compressionOptions.bitcount, compressionOptions.format);
			outputOptions.outputHandler->beginImage(size, w, h, d, f, m);
		}

		// @@ Where to do the color transform?
		// - Color transform may not be linear, so we cannot do before computing mipmaps.
		// - Should be done in linear space, that is, after gamma correction.

		if (!initMipmap(mipmap, inputOptions, w, h, d, f, m))
		{
			if (outputOptions.errorHandler != NULL)
			{
				outputOptions.errorHandler->error(Error_InvalidInput);
				return false;
			}
		}

		quantizeMipmap(mipmap, compressionOptions);

		compressMipmap(mipmap, inputOptions, compressionOptions, outputOptions);

		// Compute extents of next mipmap:
		w = max(1U, w / 2);
		h = max(1U, h / 2);
		d = max(1U, d / 2);
	}

	return true;
}

bool Compressor::Private::initMipmap(Mipmap & mipmap, const InputOptions::Private & inputOptions, uint w, uint h, uint d, uint f, uint m) const
{
	// Find image from input.
	int inputIdx = findExactMipmap(inputOptions, w, h, d, f);

	if ((inputIdx == -1 || inputOptions.convertToNormalMap) && m != 0)
	{
		// Generate from last, when mipmap not found, or normal map conversion enabled.
		downsampleMipmap(mipmap, inputOptions);
	}
	else
	{
		if (inputIdx != -1)
		{
			// If input mipmap found, then get from input.
			mipmap.setFromInput(inputOptions, inputIdx);
		}
		else
		{
			// If not found, resize closest mipmap.
			inputIdx = findClosestMipmap(inputOptions, w, h, d, f);

			if (inputIdx == -1)
			{
				return false;
			}

			mipmap.setFromInput(inputOptions, inputIdx);

			scaleMipmap(mipmap, inputOptions, w, h, d);
		}

		processInputImage(mipmap, inputOptions);
	}

	// Convert linear float image to fixed image ready for compression.
	mipmap.toFixedImage(inputOptions);

	return true;
}

int Compressor::Private::findExactMipmap(const InputOptions::Private & inputOptions, uint w, uint h, uint d, uint f) const
{
	for (int m = 0; m < int(inputOptions.mipmapCount); m++)
	{
		int idx = f * inputOptions.mipmapCount + m;
		const InputOptions::Private::InputImage & inputImage = inputOptions.images[idx];

		if (inputImage.width == int(w) && inputImage.height == int(h) && inputImage.depth == int(d))
		{
			if (inputImage.data != NULL)
			{
				return idx;
			}
			return -1;
		}
		else if (inputImage.width < int(w) || inputImage.height < int(h) || inputImage.depth < int(d))
		{
			return -1;
		}
	}

	return -1;
}

int Compressor::Private::findClosestMipmap(const InputOptions::Private & inputOptions, uint w, uint h, uint d, uint f) const
{
	int bestIdx = -1;

	for (int m = 0; m < int(inputOptions.mipmapCount); m++)
	{
		int idx = f * inputOptions.mipmapCount + m;
		const InputOptions::Private::InputImage & inputImage = inputOptions.images[idx];

		if (inputImage.data != NULL)
		{
			int difference = (inputImage.width - w) + (inputImage.height - h) + (inputImage.depth - d);

			if (difference < 0)
			{
				if (bestIdx == -1)
				{
					bestIdx = idx;
				}

				return bestIdx;
			}

			bestIdx = idx;
		}
	}

	return bestIdx;
}

// Create mipmap from the given image.
void Compressor::Private::downsampleMipmap(Mipmap & mipmap, const InputOptions::Private & inputOptions) const
{
	// Make sure that floating point linear representation is available.
	mipmap.toFloatImage(inputOptions);

	const FloatImage * floatImage = mipmap.asFloatImage();

	if (inputOptions.mipmapFilter == MipmapFilter_Box)
	{
		// Use fast downsample.
		mipmap.setImage(floatImage->fastDownSample());
	}
	else if (inputOptions.mipmapFilter == MipmapFilter_Triangle)
	{
		TriangleFilter filter;
		mipmap.setImage(floatImage->downSample(filter, (FloatImage::WrapMode)inputOptions.wrapMode));
	}
	else /*if (inputOptions.mipmapFilter == MipmapFilter_Kaiser)*/
	{
		nvDebugCheck(inputOptions.mipmapFilter == MipmapFilter_Kaiser);
		KaiserFilter filter(inputOptions.kaiserWidth);
		filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
		mipmap.setImage(floatImage->downSample(filter, (FloatImage::WrapMode)inputOptions.wrapMode));
	}

	// Normalize mipmap.
	if ((inputOptions.isNormalMap || inputOptions.convertToNormalMap) && inputOptions.normalizeMipmaps)
	{
		normalizeNormalMap(mipmap.asFloatImage());
	}
}


void Compressor::Private::scaleMipmap(Mipmap & mipmap, const InputOptions::Private & inputOptions, uint w, uint h, uint d) const
{
	mipmap.toFloatImage(inputOptions);

	// @@ Add more filters.
	// @@ Select different filters for downscaling and reconstruction.

	// Resize image. 
	BoxFilter boxFilter;
	mipmap.setImage(mipmap.asFloatImage()->resize(boxFilter, w, h, (FloatImage::WrapMode)inputOptions.wrapMode));
}


// Process an input image: Convert to normal map, normalize, or convert to linear space.
void Compressor::Private::processInputImage(Mipmap & mipmap, const InputOptions::Private & inputOptions) const
{
	if (inputOptions.convertToNormalMap)
	{
		mipmap.toFixedImage(inputOptions);

		Vector4 heightScale = inputOptions.heightFactors;
		mipmap.setImage(createNormalMap(mipmap.asFixedImage(), (FloatImage::WrapMode)inputOptions.wrapMode, heightScale, inputOptions.bumpFrequencyScale));
	}
	else if (inputOptions.isNormalMap)
	{
		if (inputOptions.normalizeMipmaps)
		{
			// If floating point image available, normalize in place.
			if (mipmap.asFloatImage() == NULL)
			{
				FloatImage * floatImage = new FloatImage(mipmap.asFixedImage());
				normalizeNormalMap(floatImage);
				mipmap.setImage(floatImage);
			}
			else
			{
				normalizeNormalMap(mipmap.asFloatImage());
				mipmap.setImage(mipmap.asFloatImage());
			}
		}
	}
	else
	{
		if (inputOptions.inputGamma != inputOptions.outputGamma)
		{
			mipmap.toFloatImage(inputOptions);
		}
	}
}


// Quantize the given mipmap according to the compression options.
void Compressor::Private::quantizeMipmap(Mipmap & mipmap, const CompressionOptions::Private & compressionOptions) const
{
	nvDebugCheck(mipmap.asFixedImage() != NULL);

	if (compressionOptions.binaryAlpha)
	{
		if (compressionOptions.enableAlphaDithering)
		{
			Quantize::FloydSteinberg_BinaryAlpha(mipmap.asMutableFixedImage(), compressionOptions.alphaThreshold);
		}
		else
		{
			Quantize::BinaryAlpha(mipmap.asMutableFixedImage(), compressionOptions.alphaThreshold);
		}
	}

	if (compressionOptions.enableColorDithering || compressionOptions.enableAlphaDithering)
	{
		uint rsize = 8;
		uint gsize = 8;
		uint bsize = 8;
		uint asize = 8;

		if (compressionOptions.enableColorDithering)
		{
			if (compressionOptions.format >= Format_DXT1 && compressionOptions.format <= Format_DXT5)
			{
				rsize = 5;
				gsize = 6;
				bsize = 5;
			}
			else if (compressionOptions.format == Format_RGB)
			{
				uint rshift, gshift, bshift;
				PixelFormat::maskShiftAndSize(compressionOptions.rmask, &rshift, &rsize);
				PixelFormat::maskShiftAndSize(compressionOptions.gmask, &gshift, &gsize);
				PixelFormat::maskShiftAndSize(compressionOptions.bmask, &bshift, &bsize);
			}
		}

		if (compressionOptions.enableAlphaDithering)
		{
			if (compressionOptions.format == Format_DXT3)
			{
				asize = 4;
			}
			else if (compressionOptions.format == Format_RGB)
			{
				uint ashift;
				PixelFormat::maskShiftAndSize(compressionOptions.amask, &ashift, &asize);
			}
		}

		if (compressionOptions.binaryAlpha)
		{
			asize = 8; // Already quantized.
		}

		Quantize::FloydSteinberg(mipmap.asMutableFixedImage(), rsize, gsize, bsize, asize);
	}
}


// Compress the given mipmap.
bool Compressor::Private::compressMipmap(const Mipmap & mipmap, const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const
{
	const Image * image = mipmap.asFixedImage();
	nvDebugCheck(image != NULL);

	FastCompressor fast;
	fast.setImage(image, inputOptions.alphaMode);

	SlowCompressor slow;
	slow.setImage(image, inputOptions.alphaMode);

	const bool useCuda = cudaEnabled && image->width() * image->height() >= 512;

	if (compressionOptions.format == Format_RGBA || compressionOptions.format == Format_RGB)
	{
		compressRGB(image, outputOptions, compressionOptions);
	}
	else if (compressionOptions.format == Format_DXT1)
	{
#if defined(HAVE_S3QUANT)
		if (compressionOptions.externalCompressor == "s3")
		{
			s3CompressDXT1(image, outputOptions);
		}
		else
#endif

#if defined(HAVE_ATITC)
			if (compressionOptions.externalCompressor == "ati")
			{
				atiCompressDXT1(image, outputOptions);
			}
			else
#endif
				if (compressionOptions.quality == Quality_Fastest)
				{
					fast.compressDXT1(outputOptions);
				}
				else
				{
					if (useCuda)
					{
						nvDebugCheck(cudaSupported);
						cuda->setImage(image, inputOptions.alphaMode);
						cuda->compressDXT1(compressionOptions, outputOptions);
					}
					else
					{
						slow.compressDXT1(compressionOptions, outputOptions);
					}
				}
	}
	else if (compressionOptions.format == Format_DXT1a)
	{
		if (compressionOptions.quality == Quality_Fastest)
		{
			fast.compressDXT1a(outputOptions);
		}
		else
		{
			if (useCuda)
			{
				nvDebugCheck(cudaSupported);
				/*cuda*/slow.compressDXT1a(compressionOptions, outputOptions);
			}
			else
			{
				slow.compressDXT1a(compressionOptions, outputOptions);
			}
		}
	}
	else if (compressionOptions.format == Format_DXT3)
	{
		if (compressionOptions.quality == Quality_Fastest)
		{
			fast.compressDXT3(outputOptions);
		}
		else
		{
			if (useCuda)
			{
				nvDebugCheck(cudaSupported);
				cuda->setImage(image, inputOptions.alphaMode);
				cuda->compressDXT3(compressionOptions, outputOptions);
			}
			else
			{
				slow.compressDXT3(compressionOptions, outputOptions);
			}
		}
	}
	else if (compressionOptions.format == Format_DXT5)
	{
		if (compressionOptions.quality == Quality_Fastest)
		{
			fast.compressDXT5(outputOptions);
		}
		else
		{
			if (useCuda)
			{
				nvDebugCheck(cudaSupported);
				cuda->setImage(image, inputOptions.alphaMode);
				cuda->compressDXT5(compressionOptions, outputOptions);
			}
			else
			{
				slow.compressDXT5(compressionOptions, outputOptions);
			}
		}
	}
	else if (compressionOptions.format == Format_DXT5n)
	{
		if (compressionOptions.quality == Quality_Fastest)
		{
			fast.compressDXT5n(outputOptions);
		}
		else
		{
			slow.compressDXT5n(compressionOptions, outputOptions);
		}
	}
	else if (compressionOptions.format == Format_BC4)
	{
		slow.compressBC4(compressionOptions, outputOptions);
	}
	else if (compressionOptions.format == Format_BC5)
	{
		slow.compressBC5(compressionOptions, outputOptions);
	}

	return true;
}


int Compressor::Private::estimateSize(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions) const
{
	const Format format = compressionOptions.format;
	const uint bitCount = compressionOptions.bitcount;

	inputOptions.computeTargetExtents();

	uint mipmapCount = inputOptions.realMipmapCount();

	int size = 0;

	for (uint f = 0; f < inputOptions.faceCount; f++)
	{
		uint w = inputOptions.targetWidth;
		uint h = inputOptions.targetHeight;
		uint d = inputOptions.targetDepth;

		for (uint m = 0; m < mipmapCount; m++)
		{
			size += computeImageSize(w, h, d, bitCount, format);

			// Compute extents of next mipmap:
			w = max(1U, w / 2);
			h = max(1U, h / 2);
			d = max(1U, d / 2);
		}
	}

	return size;
}
