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

#include <nvcore/StrLib.h>
#include <nvcore/StdStream.h>

#include <nvimage/Image.h>
#include <nvimage/DirectDrawSurface.h>

#include <nvtt/nvtt.h>

#include "cmdline.h"

#include <time.h> // clock


struct MyErrorHandler : public nvtt::ErrorHandler
{
	virtual void error(nvtt::Error e)
	{
		nvDebugBreak();
	}
};


// Set color to normal map conversion options.
void setColorToNormalMap(nvtt::InputOptions & inputOptions)
{
	inputOptions.setNormalMap(false);
	inputOptions.setConvertToNormalMap(true);
	inputOptions.setHeightEvaluation(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f, 0.0f);
	//inputOptions.setNormalFilter(1.0f, 0, 0, 0);
	//inputOptions.setNormalFilter(0.0f, 0, 0, 1);
	inputOptions.setGamma(1.0f, 1.0f);
	inputOptions.setNormalizeMipmaps(true);
}

// Set options for normal maps.
void setNormalMap(nvtt::InputOptions & inputOptions)
{
	inputOptions.setNormalMap(true);
	inputOptions.setConvertToNormalMap(false);
	inputOptions.setGamma(1.0f, 1.0f);
	inputOptions.setNormalizeMipmaps(true);
}

// Set options for color maps.
void setColorMap(nvtt::InputOptions & inputOptions)
{
	inputOptions.setNormalMap(false);
	inputOptions.setConvertToNormalMap(false);
	inputOptions.setGamma(2.2f, 2.2f);
	inputOptions.setNormalizeMipmaps(false);
}



int main(int argc, char *argv[])
{
	MyAssertHandler assertHandler;
	MyMessageHandler messageHandler;

	bool normal = false;
	bool color2normal = false;
	bool wrapRepeat = false;
	bool noMipmaps = false;
	bool fast = false;
	bool nocuda = false;
	bool silent = false;
	nvtt::Format format = nvtt::Format_BC1;

	const char * externalCompressor = NULL;

	nv::Path input;
	nv::Path output;


	// Parse arguments.
	for (int i = 1; i < argc; i++)
	{
		// Input options.
		if (strcmp("-color", argv[i]) == 0)
		{
		}
		else if (strcmp("-normal", argv[i]) == 0)
		{
			normal = true;
		}
		else if (strcmp("-tonormal", argv[i]) == 0)
		{
			color2normal = true;
		}
		else if (strcmp("-clamp", argv[i]) == 0)
		{
		}
		else if (strcmp("-repeat", argv[i]) == 0)
		{
			wrapRepeat = true;
		}
		else if (strcmp("-nomips", argv[i]) == 0)
		{
			noMipmaps = true;
		}

		// Compression options.
		else if (strcmp("-fast", argv[i]) == 0)
		{
			fast = true;
		}
		else if (strcmp("-nocuda", argv[i]) == 0)
		{
			nocuda = true;
		}
		else if (strcmp("-rgb", argv[i]) == 0)
		{
			format = nvtt::Format_RGB;
		}
		else if (strcmp("-bc1", argv[i]) == 0)
		{
			format = nvtt::Format_BC1;
		}
		else if (strcmp("-bc1a", argv[i]) == 0)
		{
			format = nvtt::Format_BC1a;
		}
		else if (strcmp("-bc2", argv[i]) == 0)
		{
			format = nvtt::Format_BC2;
		}
		else if (strcmp("-bc3", argv[i]) == 0)
		{
			format = nvtt::Format_BC3;
		}
		else if (strcmp("-bc3n", argv[i]) == 0)
		{
			format = nvtt::Format_BC3n;
		}
		else if (strcmp("-bc4", argv[i]) == 0)
		{
			format = nvtt::Format_BC4;
		}
		else if (strcmp("-bc5", argv[i]) == 0)
		{
			format = nvtt::Format_BC5;
		}

		// Undocumented option. Mainly used for testing.
		else if (strcmp("-ext", argv[i]) == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-') {
				externalCompressor = argv[i+1];
				i++;
			}
		}

		// Misc options
		else if (strcmp("-silent", argv[i]) == 0)
		{
			silent = true;
		}

		else if (argv[i][0] != '-')
		{
			input = argv[i];

			if (i+1 < argc && argv[i+1][0] != '-') {
				output = argv[i+1];
			}
			else
			{
				output.copy(input.str());
				output.stripExtension();
				output.append(".dds");
			}

			break;
		}
	}

	printf("NVIDIA Texture Tools - Copyright NVIDIA Corporation 2007\n\n");

	if (input.isNull())
	{
		printf("usage: nvttbenchmark [options] infile [outfile]\n\n");
		
		printf("Input options:\n");
		printf("  -color   \tThe input image is a color map (default).\n");
		printf("  -normal  \tThe input image is a normal map.\n");
		printf("  -tonormal\tConvert input to normal map.\n");
		printf("  -clamp   \tClamp wrapping mode (default).\n");
		printf("  -repeat  \tRepeat wrapping mode.\n");
		printf("  -nomips  \tDisable mipmap generation.\n\n");

		printf("Compression options:\n");
		printf("  -fast    \tFast compression.\n");
		printf("  -nocuda  \tDo not use cuda compressor.\n");
		printf("  -rgb     \tRGBA format\n");
		printf("  -bc1     \tBC1 format (DXT1)\n");
		printf("  -bc1a    \tBC1 format with binary alpha (DXT1a)\n");
		printf("  -bc2     \tBC2 format (DXT3)\n");
		printf("  -bc3     \tBC3 format (DXT5)\n");
		printf("  -bc3n    \tBC3 normal map format (DXT5nm)\n");
		printf("  -bc4     \tBC4 format (ATI1)\n");
		printf("  -bc5     \tBC5 format (3Dc/ATI2)\n\n");
		
		return 1;
	}

	// @@ Make sure input file exists.
	
	// Set input options.
	nvtt::InputOptions inputOptions;
	
	if (nv::strCaseCmp(input.extension(), ".dds") == 0)
	{
		// Load surface.
		nv::DirectDrawSurface dds(input);
		if (!dds.isValid())
		{
			fprintf(stderr, "The file '%s' is not a valid DDS file.\n", input.str());
			return 1;
		}
		
		if (!dds.isSupported() || dds.isTexture3D())
		{
			fprintf(stderr, "The file '%s' is not a supported DDS file.\n", input.str());
			return 1;
		}
		
		uint faceCount;
		if (dds.isTexture2D())
		{
			inputOptions.setTextureLayout(nvtt::TextureType_2D, dds.width(), dds.height());
			faceCount = 1;
		}
		else 
		{
			nvDebugCheck(dds.isTextureCube());
			inputOptions.setTextureLayout(nvtt::TextureType_Cube, dds.width(), dds.height());
			faceCount = 6;
		}
		
		uint mipmapCount = dds.mipmapCount();
		
		nv::Image mipmap;
		
		for (uint f = 0; f < faceCount; f++)
		{
			for (uint m = 0; m <= mipmapCount; m++)
			{
				dds.mipmap(&mipmap, f, m);
				
				inputOptions.setMipmapData(mipmap.pixels(), mipmap.width(), mipmap.height(), 1, f, m);
			}
		}
	}
	else
	{
		// Regular image.
		nv::Image image;
		if (!image.load(input))
		{
			fprintf(stderr, "The file '%s' is not a supported image type.\n", input.str());
			return 1;
		}
		
		inputOptions.setTextureLayout(nvtt::TextureType_2D, image.width(), image.height());
		inputOptions.setMipmapData(image.pixels(), image.width(), image.height());
	}

	if (fast)
	{
		inputOptions.setMipmapping(true, nvtt::MipmapFilter_Box);
	}
	else
	{
		inputOptions.setMipmapping(true, nvtt::MipmapFilter_Box);
		//inputOptions.setMipmapping(true, nvtt::MipmapFilter_Kaiser);
	}

	if (wrapRepeat)
	{
		inputOptions.setWrapMode(nvtt::WrapMode_Repeat);
	}
	else
	{
		inputOptions.setWrapMode(nvtt::WrapMode_Clamp);
	}

	if (normal)
	{
		setNormalMap(inputOptions);
	}
	else if (color2normal)
	{
		setColorToNormalMap(inputOptions);
	}
	else
	{
		setColorMap(inputOptions);
	}
	
	if (noMipmaps)
	{
		inputOptions.setMipmapping(false);
	}
	

	nvtt::CompressionOptions compressionOptions;
	compressionOptions.setFormat(format);
	if (fast)
	{
		compressionOptions.setQuality(nvtt::Quality_Fastest);
	}
	else
	{
		compressionOptions.setQuality(nvtt::Quality_Normal);
		//compressionOptions.setQuality(nvtt::Quality_Production, 0.5f);
		//compressionOptions.setQuality(nvtt::Quality_Highest);
	}
	compressionOptions.enableHardwareCompression(!nocuda);
	compressionOptions.setColorWeights(1, 1, 1);

	if (externalCompressor != NULL)
	{
		compressionOptions.setExternalCompressor(externalCompressor);
	}	
	
	
	MyErrorHandler errorHandler;
	nvtt::OutputOptions outputOptions(NULL, &errorHandler);
	
//	printf("Press ENTER.\n");
//	fflush(stdout);
//	getchar();

	clock_t start = clock();
	
	const int iterationCount = 20;
	for (int i = 0; i < iterationCount; i++)
	{
		nvtt::compress(inputOptions, outputOptions, compressionOptions);
	}

	clock_t end = clock();
	
	float seconds = float(end-start) / CLOCKS_PER_SEC
	printf("total time taken: %.3f seconds\n", seconds);
	printf("time taken per texture: %.3f seconds\n", seconds / iterationCount);
	printf("textures per second: %.3f T/s\n", iterationCount / seconds);
	
	return 0;
}

