// This code is in the public domain -- castanyo@yahoo.es

/*
http://www.visgraf.impa.br/Courses/ip00/proj/Dithering1/floyd_steinberg_dithering.html
http://www.gamedev.net/reference/articles/article341.asp

@@ Look at LPS: http://www.cs.rit.edu/~pga/pics2000/i.html
 
This is a really nice guide to dithering algorithms:
http://www.efg2.com/Lab/Library/ImageProcessing/DHALF.TXT

@@ This code needs to be reviewed, I'm not sure it's correct.
*/

#include <nvimage/Quantize.h>
#include <nvimage/Image.h>
#include <nvimage/PixelFormat.h>

#include <nvmath/Color.h>

#include <nvcore/Containers.h> // swap


using namespace nv;


// Simple quantization.
void nv::Quantize::BinaryAlpha( Image * image, int alpha_threshold /*= 127*/ )
{
	nvCheck(image != NULL);
	
	const uint w = image->width();
	const uint h = image->height();
	
	for(uint y = 0; y < h; y++) {
		for(uint x = 0; x < w; x++) {
			
			Color32 pixel = image->pixel(x, y);
			
			// Convert color.
			if( pixel.a > alpha_threshold ) pixel.a = 255;
			else pixel.a = 0;
			
			// Store color.
			image->pixel(x, y) = pixel;
		}
	}
}


// Simple quantization.
void nv::Quantize::RGB16( Image * image )
{
	Truncate(image, 5, 6, 5, 8);
}

// Alpha quantization.
void nv::Quantize::Alpha4( Image * image )
{
	Truncate(image, 8, 8, 8, 4);
}


// Error diffusion. Floyd Steinberg.
void nv::Quantize::FloydSteinberg_RGB16( Image * image )
{
	FloydSteinberg(image, 5, 6, 5, 8);
}


// Error diffusion. Floyd Steinberg.
void nv::Quantize::FloydSteinberg_BinaryAlpha( Image * image, int alpha_threshold /*= 127*/ ) 
{
	nvCheck(image != NULL);
	
	const uint w = image->width();
	const uint h = image->height();
	
	// @@ Use fixed point?
	float * row0 = new float[(w+2)];
	float * row1 = new float[(w+2)];
	memset(row0, 0, sizeof(float)*(w+2));
	memset(row1, 0, sizeof(float)*(w+2));
	
	for(uint y = 0; y < h; y++) {
		for(uint x = 0; x < w; x++) {
			
			Color32 pixel = image->pixel(x, y);
			
			// Add error.
			int alpha = int(pixel.a) + int(row0[1+x]);
			
			// Convert color.
			if( alpha > alpha_threshold ) pixel.a = 255;
			else pixel.a = 0;
			
			// Store color.
			image->pixel(x, y) = pixel;
			
			// Compute new error.
			float diff = float(alpha - pixel.a);
			
			// Propagate new error.
			row0[1+x+1] += 7.0f / 16.0f * diff;
			row1[1+x-1] += 3.0f / 16.0f * diff;
			row1[1+x+0] += 5.0f / 16.0f * diff;
			row1[1+x+1] += 1.0f / 16.0f * diff;
		}
		
		swap(row0, row1);
		memset(row1, 0, sizeof(float)*(w+2));
	}
	
	delete [] row0;
	delete [] row1;
}


// Error diffusion. Floyd Steinberg.
void nv::Quantize::FloydSteinberg_Alpha4( Image * image )
{
	FloydSteinberg(image, 8, 8, 8, 4);
}


void nv::Quantize::Truncate(Image * image, uint rsize, uint gsize, uint bsize, uint asize)
{
	nvCheck(image != NULL);
	
	const uint w = image->width();
	const uint h = image->height();
	
	for(uint y = 0; y < h; y++) {
		for(uint x = 0; x < w; x++) {
			
			Color32 pixel = image->pixel(x, y);

			// Convert to our desired size, and reconstruct.
			pixel.r = PixelFormat::convert(pixel.r, 8, rsize);
			pixel.r = PixelFormat::convert(pixel.r, rsize, 8);

			pixel.g = PixelFormat::convert(pixel.g, 8, gsize);
			pixel.g = PixelFormat::convert(pixel.g, gsize, 8);

			pixel.b = PixelFormat::convert(pixel.b, 8, bsize);
			pixel.b = PixelFormat::convert(pixel.b, bsize, 8);

			pixel.a = PixelFormat::convert(pixel.a, 8, asize);
			pixel.a = PixelFormat::convert(pixel.a, asize, 8);

			// Store color.
			image->pixel(x, y) = pixel;
		}
	}
}


// Error diffusion. Floyd Steinberg.
void nv::Quantize::FloydSteinberg(Image * image, uint rsize, uint gsize, uint bsize, uint asize)
{
	nvCheck(image != NULL);
	
	const uint w = image->width();
	const uint h = image->height();
	
	Vector4 * row0 = new Vector4[w+2];
	Vector4 * row1 = new Vector4[w+2];
	memset(row0, 0, sizeof(Vector4)*(w+2));
	memset(row1, 0, sizeof(Vector4)*(w+2));
	
	for (uint y = 0; y < h; y++) {
		for (uint x = 0; x < w; x++) {
			
			Color32 pixel = image->pixel(x, y);

			// Add error.
			pixel.r = clamp(int(pixel.r) + int(row0[1+x].x()), 0, 255);
			pixel.g = clamp(int(pixel.g) + int(row0[1+x].y()), 0, 255);
			pixel.b = clamp(int(pixel.b) + int(row0[1+x].z()), 0, 255);
			pixel.a = clamp(int(pixel.a) + int(row0[1+x].w()), 0, 255);
			
			int r = pixel.r;
			int g = pixel.g;
			int b = pixel.b;
			int a = pixel.a;

			// Convert to our desired size, and reconstruct.
			r = PixelFormat::convert(r, 8, rsize);
			r = PixelFormat::convert(r, rsize, 8);

			g = PixelFormat::convert(g, 8, gsize);
			g = PixelFormat::convert(g, gsize, 8);

			b = PixelFormat::convert(b, 8, bsize);
			b = PixelFormat::convert(b, bsize, 8);

			a = PixelFormat::convert(a, 8, asize);
			a = PixelFormat::convert(a, asize, 8);

			// Store color.
			image->pixel(x, y) = Color32(r, g, b, a);
			
			// Compute new error.
			Vector4 diff(float(int(pixel.r) - r), float(int(pixel.g) - g), float(int(pixel.b) - b), float(int(pixel.a) - a));
			
			// Propagate new error.
			row0[1+x+1] += 7.0f / 16.0f * diff;
			row1[1+x-1] += 3.0f / 16.0f * diff;
			row1[1+x+0] += 5.0f / 16.0f * diff;
			row1[1+x+1] += 1.0f / 16.0f * diff;
		}
		
		swap(row0, row1);
		memset(row1, 0, sizeof(Vector4)*(w+2));
	}
	
	delete [] row0;
	delete [] row1;
}
