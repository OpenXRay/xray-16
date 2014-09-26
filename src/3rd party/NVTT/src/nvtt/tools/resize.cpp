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
#include <nvcore/StrLib.h>
#include <nvcore/StdStream.h>
#include <nvcore/Containers.h>

#include <nvimage/Image.h>
#include <nvimage/ImageIO.h>
#include <nvimage/FloatImage.h>
#include <nvimage/Filter.h>
#include <nvimage/DirectDrawSurface.h>

#include <nvmath/Color.h>
#include <nvmath/Vector.h>

#include <math.h>

#include "cmdline.h"

static bool loadImage(nv::Image & image, const char * fileName)
{
	if (nv::strCaseCmp(nv::Path::extension(fileName), ".dds") == 0)
	{
		nv::DirectDrawSurface dds(fileName);
		if (!dds.isValid())
		{
			printf("The file '%s' is not a valid DDS file.\n", fileName);
			return false;
		}
		
		dds.mipmap(&image, 0, 0); // get first image
	}
	else
	{
		// Regular image.
		if (!image.load(fileName))
		{
			printf("The file '%s' is not a supported image type.\n", fileName);
			return false;
		}
	}

	return true;
}


int main(int argc, char *argv[])
{
	//MyAssertHandler assertHandler;
	MyMessageHandler messageHandler;

	float scale = 0.5f;
	float gamma = 2.2f;
	nv::AutoPtr<nv::Filter> filter;
	nv::Path input;
	nv::Path output;
	
	nv::FloatImage::WrapMode wrapMode = nv::FloatImage::WrapMode_Mirror;

	// Parse arguments.
	for (int i = 1; i < argc; i++)
	{
		// Input options.
		if (strcmp("-s", argv[i]) == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-') {
				scale = (float)atof(argv[i+1]);
				i++;
			}
		}
		else if (strcmp("-g", argv[i]) == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-') {
				gamma = (float)atof(argv[i+1]);
				i++;
			}
		}
		else if (strcmp("-f", argv[i]) == 0)
		{
			if (i+1 == argc) break;
			i++;
			
			if (strcmp("box", argv[i]) == 0) filter = new nv::BoxFilter();
			else if (strcmp("triangle", argv[i]) == 0) filter = new nv::TriangleFilter();
			else if (strcmp("quadratic", argv[i]) == 0) filter = new nv::QuadraticFilter();
			else if (strcmp("bspline", argv[i]) == 0) filter = new nv::BSplineFilter();
			else if (strcmp("mitchell", argv[i]) == 0) filter = new nv::MitchellFilter();
			else if (strcmp("lanczos", argv[i]) == 0) filter = new nv::LanczosFilter();
			else if (strcmp("kaiser", argv[i]) == 0) {
				filter = new nv::KaiserFilter(3);
				((nv::KaiserFilter *)filter.ptr())->setParameters(4.0f, 1.0f);
			}
		}
		else if (strcmp("-w", argv[i]) == 0)
		{
			if (i+1 == argc) break;
			i++;

			if (strcmp("mirror", argv[i]) == 0) wrapMode = nv::FloatImage::WrapMode_Mirror;
			else if (strcmp("repeat", argv[i]) == 0) wrapMode = nv::FloatImage::WrapMode_Repeat;
			else if (strcmp("clamp", argv[i]) == 0) wrapMode = nv::FloatImage::WrapMode_Clamp;
		}
		else if (argv[i][0] != '-')
		{
			input = argv[i];

			if (i+1 < argc && argv[i+1][0] != '-') {
				output = argv[i+1];
			}

			break;
		}
	}

	if (input.isNull() || output.isNull())
	{
		printf("NVIDIA Texture Tools - Copyright NVIDIA Corporation 2007\n\n");	
		
		printf("usage: nvzoom [options] input [output]\n\n");
		
		printf("Options:\n");
		printf("  -s scale     Scale factor (default = 0.5)\n");
		printf("  -g gamma     Gamma correction (default = 2.2)\n");
		printf("  -f filter    One of the following: (default = 'box')\n");
		printf("                * box\n");
		printf("                * triangle\n");
		printf("                * quadratic\n");
		printf("                * bspline\n");
		printf("                * mitchell\n");
		printf("                * lanczos\n");
		printf("                * kaiser\n");
		printf("  -w mode      One of the following: (default = 'mirror')\n");
		printf("                * mirror\n");
		printf("                * repeat\n");
		printf("                * clamp\n");

		return 1;
	}
	
	if (filter == NULL)
	{
		filter = new nv::BoxFilter();
	}

	nv::Image image;
	if (!loadImage(image, input)) return 0;

	nv::FloatImage fimage(&image);
	fimage.toLinear(0, 3, gamma);
	
	nv::AutoPtr<nv::FloatImage> fresult(fimage.resize(*filter, uint(image.width() * scale), uint(image.height() * scale), wrapMode));
	
	nv::AutoPtr<nv::Image> result(fresult->createImageGammaCorrect(gamma));
	result->setFormat(nv::Image::Format_ARGB);

	nv::StdOutputStream stream(output);
	nv::ImageIO::saveTGA(stream, result.ptr());	// @@ Add generic save function. Add support for png too.
	
	return 0;
}

