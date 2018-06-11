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
#include <nvcore/Containers.h> // max
#include <nvcore/StdStream.h>

#include <nvimage/DirectDrawSurface.h>
#include <nvimage/ColorBlock.h>
#include <nvimage/Image.h>
#include <nvimage/BlockDXT.h>
#include <nvimage/PixelFormat.h>

#include <string.h> // memset


using namespace nv;

#if !defined(MAKEFOURCC)
#	define MAKEFOURCC(ch0, ch1, ch2, ch3) \
		(uint(uint8(ch0)) | (uint(uint8(ch1)) << 8) | \
		(uint(uint8(ch2)) << 16) | (uint(uint8(ch3)) << 24 ))
#endif

namespace
{
	static const uint FOURCC_DDS = MAKEFOURCC('D', 'D', 'S', ' ');
	static const uint FOURCC_DXT1 = MAKEFOURCC('D', 'X', 'T', '1');
	static const uint FOURCC_DXT2 = MAKEFOURCC('D', 'X', 'T', '2');
	static const uint FOURCC_DXT3 = MAKEFOURCC('D', 'X', 'T', '3');
	static const uint FOURCC_DXT4 = MAKEFOURCC('D', 'X', 'T', '4');
	static const uint FOURCC_DXT5 = MAKEFOURCC('D', 'X', 'T', '5');
	static const uint FOURCC_RXGB = MAKEFOURCC('R', 'X', 'G', 'B');
	static const uint FOURCC_ATI1 = MAKEFOURCC('A', 'T', 'I', '1');
	static const uint FOURCC_ATI2 = MAKEFOURCC('A', 'T', 'I', '2');

	static const uint FOURCC_A2XY = MAKEFOURCC('A', '2', 'X', 'Y');
	
	static const uint FOURCC_DX10 = MAKEFOURCC('D', 'X', '1', '0');

	// 32 bit RGB formats.
	static const uint D3DFMT_R8G8B8 = 20;
	static const uint D3DFMT_A8R8G8B8 = 21;
	static const uint D3DFMT_X8R8G8B8 = 22;
	static const uint D3DFMT_R5G6B5 = 23;
	static const uint D3DFMT_X1R5G5B5 = 24;
	static const uint D3DFMT_A1R5G5B5 = 25;
	static const uint D3DFMT_A4R4G4B4 = 26;
	static const uint D3DFMT_R3G3B2 = 27;
	static const uint D3DFMT_A8 = 28;
	static const uint D3DFMT_A8R3G3B2 = 29;
	static const uint D3DFMT_X4R4G4B4 = 30;
	static const uint D3DFMT_A2B10G10R10 = 31;
	static const uint D3DFMT_A8B8G8R8 = 32;
	static const uint D3DFMT_X8B8G8R8 = 33;
	static const uint D3DFMT_G16R16 = 34;
	static const uint D3DFMT_A2R10G10B10 = 35;

	static const uint D3DFMT_A16B16G16R16 = 36;

	// Palette formats.
	static const uint D3DFMT_A8P8 = 40;
	static const uint D3DFMT_P8 = 41;

	// Luminance formats.
	static const uint D3DFMT_L8 = 50;
	static const uint D3DFMT_A8L8 = 51;
	static const uint D3DFMT_A4L4 = 52;
	static const uint D3DFMT_L16 = 81;

	// Floating point formats
	static const uint D3DFMT_R16F = 111;
	static const uint D3DFMT_G16R16F = 112;
	static const uint D3DFMT_A16B16G16R16F = 113;
	static const uint D3DFMT_R32F = 114;
	static const uint D3DFMT_G32R32F = 115;
	static const uint D3DFMT_A32B32G32R32F = 116;

	static const uint DDSD_CAPS = 0x00000001U;
	static const uint DDSD_PIXELFORMAT = 0x00001000U;
	static const uint DDSD_WIDTH = 0x00000004U;
	static const uint DDSD_HEIGHT = 0x00000002U;
	static const uint DDSD_PITCH = 0x00000008U;
	static const uint DDSD_MIPMAPCOUNT = 0x00020000U;
	static const uint DDSD_LINEARSIZE = 0x00080000U;
	static const uint DDSD_DEPTH = 0x00800000U;

	static const uint DDSCAPS_COMPLEX = 0x00000008U;
	static const uint DDSCAPS_TEXTURE = 0x00001000U;
	static const uint DDSCAPS_MIPMAP = 0x00400000U;
	static const uint DDSCAPS2_VOLUME = 0x00200000U;
	static const uint DDSCAPS2_CUBEMAP = 0x00000200U;

	static const uint DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400U;
	static const uint DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800U;
	static const uint DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000U;
	static const uint DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000U;
	static const uint DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000U;
	static const uint DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000U;
	static const uint DDSCAPS2_CUBEMAP_ALL_FACES = 0x0000FC00U;

	static const uint DDPF_ALPHAPIXELS = 0x00000001U;
	static const uint DDPF_ALPHA = 0x00000002U;
	static const uint DDPF_FOURCC = 0x00000004U;
	static const uint DDPF_RGB = 0x00000040U;
	static const uint DDPF_PALETTEINDEXED1 = 0x00000800U;
	static const uint DDPF_PALETTEINDEXED2 = 0x00001000U;
	static const uint DDPF_PALETTEINDEXED4 = 0x00000008U;
	static const uint DDPF_PALETTEINDEXED8 = 0x00000020U;
	static const uint DDPF_LUMINANCE = 0x00020000U;
	static const uint DDPF_ALPHAPREMULT = 0x00008000U;
	static const uint DDPF_NORMAL = 0x80000000U;	// @@ Custom nv flag.

	// DX10 formats.
	enum DXGI_FORMAT
	{
		DXGI_FORMAT_UNKNOWN = 0,
		
		DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
		DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
		DXGI_FORMAT_R32G32B32A32_UINT = 3,
		DXGI_FORMAT_R32G32B32A32_SINT = 4,
		
		DXGI_FORMAT_R32G32B32_TYPELESS = 5,
		DXGI_FORMAT_R32G32B32_FLOAT = 6,
		DXGI_FORMAT_R32G32B32_UINT = 7,
		DXGI_FORMAT_R32G32B32_SINT = 8,
		
		DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
		DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
		DXGI_FORMAT_R16G16B16A16_UNORM = 11,
		DXGI_FORMAT_R16G16B16A16_UINT = 12,
		DXGI_FORMAT_R16G16B16A16_SNORM = 13,
		DXGI_FORMAT_R16G16B16A16_SINT = 14,
		
		DXGI_FORMAT_R32G32_TYPELESS = 15,
		DXGI_FORMAT_R32G32_FLOAT = 16,
		DXGI_FORMAT_R32G32_UINT = 17,
		DXGI_FORMAT_R32G32_SINT = 18,
		
		DXGI_FORMAT_R32G8X24_TYPELESS = 19,
		DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
		DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
		DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
		
		DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
		DXGI_FORMAT_R10G10B10A2_UNORM = 24,
		DXGI_FORMAT_R10G10B10A2_UINT = 25,
		
		DXGI_FORMAT_R11G11B10_FLOAT = 26,
		
		DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
		DXGI_FORMAT_R8G8B8A8_UNORM = 28,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
		DXGI_FORMAT_R8G8B8A8_UINT = 30,
		DXGI_FORMAT_R8G8B8A8_SNORM = 31,
		DXGI_FORMAT_R8G8B8A8_SINT = 32,
		
		DXGI_FORMAT_R16G16_TYPELESS = 33,
		DXGI_FORMAT_R16G16_FLOAT = 34,
		DXGI_FORMAT_R16G16_UNORM = 35,
		DXGI_FORMAT_R16G16_UINT = 36,
		DXGI_FORMAT_R16G16_SNORM = 37,
		DXGI_FORMAT_R16G16_SINT = 38,
		
		DXGI_FORMAT_R32_TYPELESS = 39,
		DXGI_FORMAT_D32_FLOAT = 40,
		DXGI_FORMAT_R32_FLOAT = 41,
		DXGI_FORMAT_R32_UINT = 42,
		DXGI_FORMAT_R32_SINT = 43,
		
		DXGI_FORMAT_R24G8_TYPELESS = 44,
		DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
		DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
		
		DXGI_FORMAT_R8G8_TYPELESS = 48,
		DXGI_FORMAT_R8G8_UNORM = 49,
		DXGI_FORMAT_R8G8_UINT = 50,
		DXGI_FORMAT_R8G8_SNORM = 51,
		DXGI_FORMAT_R8G8_SINT = 52,
		
		DXGI_FORMAT_R16_TYPELESS = 53,
		DXGI_FORMAT_R16_FLOAT = 54,
		DXGI_FORMAT_D16_UNORM = 55,
		DXGI_FORMAT_R16_UNORM = 56,
		DXGI_FORMAT_R16_UINT = 57,
		DXGI_FORMAT_R16_SNORM = 58,
		DXGI_FORMAT_R16_SINT = 59,
		
		DXGI_FORMAT_R8_TYPELESS = 60,
		DXGI_FORMAT_R8_UNORM = 61,
		DXGI_FORMAT_R8_UINT = 62,
		DXGI_FORMAT_R8_SNORM = 63,
		DXGI_FORMAT_R8_SINT = 64,
		DXGI_FORMAT_A8_UNORM = 65,
		
		DXGI_FORMAT_R1_UNORM = 66,
		
		DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
		
		DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
		DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
		
		DXGI_FORMAT_BC1_TYPELESS = 70,
		DXGI_FORMAT_BC1_UNORM = 71,
		DXGI_FORMAT_BC1_UNORM_SRGB = 72,
		
		DXGI_FORMAT_BC2_TYPELESS = 73,
		DXGI_FORMAT_BC2_UNORM = 74,
		DXGI_FORMAT_BC2_UNORM_SRGB = 75,
		
		DXGI_FORMAT_BC3_TYPELESS = 76,
		DXGI_FORMAT_BC3_UNORM = 77,
		DXGI_FORMAT_BC3_UNORM_SRGB = 78,
		
		DXGI_FORMAT_BC4_TYPELESS = 79,
		DXGI_FORMAT_BC4_UNORM = 80,
		DXGI_FORMAT_BC4_SNORM = 81,
		
		DXGI_FORMAT_BC5_TYPELESS = 82,
		DXGI_FORMAT_BC5_UNORM = 83,
		DXGI_FORMAT_BC5_SNORM = 84,
		
		DXGI_FORMAT_B5G6R5_UNORM = 85,
		DXGI_FORMAT_B5G5R5A1_UNORM = 86,
		DXGI_FORMAT_B8G8R8A8_UNORM = 87,
		DXGI_FORMAT_B8G8R8X8_UNORM = 88,
	};

	enum D3D10_RESOURCE_DIMENSION
	{
		D3D10_RESOURCE_DIMENSION_UNKNOWN = 0,
		D3D10_RESOURCE_DIMENSION_BUFFER = 1,
		D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
		D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
		D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4,
	};


	const char * getDxgiFormatString(DXGI_FORMAT dxgiFormat)
	{
#define CASE(format) case DXGI_FORMAT_##format: return #format
		switch(dxgiFormat)
		{
			CASE(UNKNOWN);
			
			CASE(R32G32B32A32_TYPELESS);
			CASE(R32G32B32A32_FLOAT);
			CASE(R32G32B32A32_UINT);
			CASE(R32G32B32A32_SINT);
			
			CASE(R32G32B32_TYPELESS);
			CASE(R32G32B32_FLOAT);
			CASE(R32G32B32_UINT);
			CASE(R32G32B32_SINT);
			
			CASE(R16G16B16A16_TYPELESS);
			CASE(R16G16B16A16_FLOAT);
			CASE(R16G16B16A16_UNORM);
			CASE(R16G16B16A16_UINT);
			CASE(R16G16B16A16_SNORM);
			CASE(R16G16B16A16_SINT);
			
			CASE(R32G32_TYPELESS);
			CASE(R32G32_FLOAT);
			CASE(R32G32_UINT);
			CASE(R32G32_SINT);
			
			CASE(R32G8X24_TYPELESS);
			CASE(D32_FLOAT_S8X24_UINT);
			CASE(R32_FLOAT_X8X24_TYPELESS);
			CASE(X32_TYPELESS_G8X24_UINT);
			
			CASE(R10G10B10A2_TYPELESS);
			CASE(R10G10B10A2_UNORM);
			CASE(R10G10B10A2_UINT);
			
			CASE(R11G11B10_FLOAT);
			
			CASE(R8G8B8A8_TYPELESS);
			CASE(R8G8B8A8_UNORM);
			CASE(R8G8B8A8_UNORM_SRGB);
			CASE(R8G8B8A8_UINT);
			CASE(R8G8B8A8_SNORM);
			CASE(R8G8B8A8_SINT);
			
			CASE(R16G16_TYPELESS);
			CASE(R16G16_FLOAT);
			CASE(R16G16_UNORM);
			CASE(R16G16_UINT);
			CASE(R16G16_SNORM);
			CASE(R16G16_SINT);
			
			CASE(R32_TYPELESS);
			CASE(D32_FLOAT);
			CASE(R32_FLOAT);
			CASE(R32_UINT);
			CASE(R32_SINT);
			
			CASE(R24G8_TYPELESS);
			CASE(D24_UNORM_S8_UINT);
			CASE(R24_UNORM_X8_TYPELESS);
			CASE(X24_TYPELESS_G8_UINT);
			
			CASE(R8G8_TYPELESS);
			CASE(R8G8_UNORM);
			CASE(R8G8_UINT);
			CASE(R8G8_SNORM);
			CASE(R8G8_SINT);
			
			CASE(R16_TYPELESS);
			CASE(R16_FLOAT);
			CASE(D16_UNORM);
			CASE(R16_UNORM);
			CASE(R16_UINT);
			CASE(R16_SNORM);
			CASE(R16_SINT);
			
			CASE(R8_TYPELESS);
			CASE(R8_UNORM);
			CASE(R8_UINT);
			CASE(R8_SNORM);
			CASE(R8_SINT);
			CASE(A8_UNORM);

			CASE(R1_UNORM);
		
			CASE(R9G9B9E5_SHAREDEXP);
			
			CASE(R8G8_B8G8_UNORM);
			CASE(G8R8_G8B8_UNORM);

			CASE(BC1_TYPELESS);
			CASE(BC1_UNORM);
			CASE(BC1_UNORM_SRGB);
		
			CASE(BC2_TYPELESS);
			CASE(BC2_UNORM);
			CASE(BC2_UNORM_SRGB);
		
			CASE(BC3_TYPELESS);
			CASE(BC3_UNORM);
			CASE(BC3_UNORM_SRGB);
		
			CASE(BC4_TYPELESS);
			CASE(BC4_UNORM);
			CASE(BC4_SNORM);
		
			CASE(BC5_TYPELESS);
			CASE(BC5_UNORM);
			CASE(BC5_SNORM);

			CASE(B5G6R5_UNORM);
			CASE(B5G5R5A1_UNORM);
			CASE(B8G8R8A8_UNORM);
			CASE(B8G8R8X8_UNORM);

			default: 
				return "UNKNOWN";
		}
#undef CASE
	}
	
	const char * getD3d10ResourceDimensionString(D3D10_RESOURCE_DIMENSION resourceDimension)
	{
		switch(resourceDimension)
		{
			default:
			case D3D10_RESOURCE_DIMENSION_UNKNOWN: return "UNKNOWN";
			case D3D10_RESOURCE_DIMENSION_BUFFER: return "BUFFER";
			case D3D10_RESOURCE_DIMENSION_TEXTURE1D: return "TEXTURE1D";
			case D3D10_RESOURCE_DIMENSION_TEXTURE2D: return "TEXTURE2D";
			case D3D10_RESOURCE_DIMENSION_TEXTURE3D: return "TEXTURE3D";
		}
	}

} // namespace

namespace nv
{
	static Stream & operator<< (Stream & s, DDSPixelFormat & pf)
	{
		nvStaticCheck(sizeof(DDSPixelFormat) == 32);
		s << pf.size;
		s << pf.flags;
		s << pf.fourcc;
		s << pf.bitcount;
		s << pf.rmask;
		s << pf.gmask;
		s << pf.bmask;
		s << pf.amask;
		return s;
	}

	static Stream & operator<< (Stream & s, DDSCaps & caps)
	{
		nvStaticCheck(sizeof(DDSCaps) == 16);
		s << caps.caps1;
		s << caps.caps2;
		s << caps.caps3;
		s << caps.caps4;
		return s;
	}

	static Stream & operator<< (Stream & s, DDSHeader10 & header)
	{
		nvStaticCheck(sizeof(DDSHeader10) == 20);
		s << header.dxgiFormat;
		s << header.resourceDimension;
		s << header.miscFlag;
		s << header.arraySize;
		s << header.reserved;
		return s;
	}

	Stream & operator<< (Stream & s, DDSHeader & header)
	{
		nvStaticCheck(sizeof(DDSHeader) == 148);
		s << header.fourcc;
		s << header.size;
		s << header.flags;
		s << header.height;
		s << header.width;
		s << header.pitch;
		s << header.depth;
		s << header.mipmapcount;
		s.serialize(header.reserved, 11 * sizeof(uint));
		s << header.pf;
		s << header.caps;
		s << header.notused;
		
		if (header.hasDX10Header())
		{
			s << header.header10;
		}
		
		return s;
	}

} // nv namespace

/* Not used!
namespace
{
	struct FormatDescriptor
	{
		uint format;
		uint bitcount;
		uint rmask;
		uint gmask;
		uint bmask;
		uint amask;
	};

	static const FormatDescriptor s_d3dFormats[] =
	{
		{ D3DFMT_R8G8B8,		24, 0xFF0000,   0xFF00,	    0xFF,       0 },
		{ D3DFMT_A8R8G8B8,		32, 0xFF0000,   0xFF00,     0xFF,       0xFF000000 },  // DXGI_FORMAT_B8G8R8A8_UNORM
		{ D3DFMT_X8R8G8B8,		32, 0xFF0000,   0xFF00,     0xFF,       0 },           // DXGI_FORMAT_B8G8R8X8_UNORM
		{ D3DFMT_R5G6B5,		16,	0xF800,     0x7E0,      0x1F,       0 },           // DXGI_FORMAT_B5G6R5_UNORM
		{ D3DFMT_X1R5G5B5,		16, 0x7C00,     0x3E0,      0x1F,       0 },
		{ D3DFMT_A1R5G5B5,		16, 0x7C00,     0x3E0,      0x1F,       0x8000 },      // DXGI_FORMAT_B5G5R5A1_UNORM
		{ D3DFMT_A4R4G4B4,		16, 0xF00,      0xF0,       0xF,        0xF000 },
		{ D3DFMT_R3G3B2,		8,  0xE0,       0x1C,       0x3,	    0 },
		{ D3DFMT_A8,			8,  0,          0,          0,		    8 },           // DXGI_FORMAT_A8_UNORM
		{ D3DFMT_A8R3G3B2,		16, 0xE0,       0x1C,       0x3,        0xFF00 },
		{ D3DFMT_X4R4G4B4,		16, 0xF00,      0xF0,       0xF,        0 },
		{ D3DFMT_A2B10G10R10,	32, 0x3FF,      0xFFC00,    0x3FF00000, 0xC0000000 },  // DXGI_FORMAT_R10G10B10A2
		{ D3DFMT_A8B8G8R8,		32, 0xFF,       0xFF00,     0xFF0000,   0xFF000000 },  // DXGI_FORMAT_R8G8B8A8_UNORM
		{ D3DFMT_X8B8G8R8,		32, 0xFF,       0xFF00,     0xFF0000,   0 },
		{ D3DFMT_G16R16,		32, 0xFFFF,     0xFFFF0000, 0,          0 },           // DXGI_FORMAT_R16G16_UNORM
		{ D3DFMT_A2R10G10B10,	32, 0x3FF00000, 0xFFC00,    0x3FF,      0xC0000000 },

		{ D3DFMT_L8,			8,  8,          0,          0,          0 },           // DXGI_FORMAT_R8_UNORM 
		{ D3DFMT_L16,			16, 16,         0,          0,          0 },           // DXGI_FORMAT_R16_UNORM
	};

	static const uint s_d3dFormatCount = sizeof(s_d3dFormats) / sizeof(s_d3dFormats[0]);
	
	static uint findD3D9Format(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask)
	{
		for (int i = 0; i < s_d3dFormatCount; i++)
		{
			if (s_d3dFormats[i].bitcount == bitcount &&
				s_d3dFormats[i].rmask == rmask &&
				s_d3dFormats[i].gmask == gmask &&
				s_d3dFormats[i].bmask == bmask &&
				s_d3dFormats[i].amask == amask)
			{
				return s_d3dFormats[i].format;
			}
		}

		return 0;
	}

} // nv namespace
*/

DDSHeader::DDSHeader()
{
	this->fourcc = FOURCC_DDS;
	this->size = 124;
	this->flags  = (DDSD_CAPS|DDSD_PIXELFORMAT);
	this->height = 0;
	this->width = 0;
	this->pitch = 0;
	this->depth = 0;
	this->mipmapcount = 0;
	memset(this->reserved, 0, sizeof(this->reserved));

	// Store version information on the reserved header attributes.
	this->reserved[9] = MAKEFOURCC('N', 'V', 'T', 'T');
	this->reserved[10] = (2 << 16) | (0 << 8) | (8);	// major.minor.revision

	this->pf.size = 32;
	this->pf.flags = 0;
	this->pf.fourcc = 0;
	this->pf.bitcount = 0;
	this->pf.rmask = 0;
	this->pf.gmask = 0;
	this->pf.bmask = 0;
	this->pf.amask = 0;
	this->caps.caps1 = DDSCAPS_TEXTURE;
	this->caps.caps2 = 0;
	this->caps.caps3 = 0;
	this->caps.caps4 = 0;
	this->notused = 0;
	
	this->header10.dxgiFormat = DXGI_FORMAT_UNKNOWN;
	this->header10.resourceDimension = D3D10_RESOURCE_DIMENSION_UNKNOWN;
	this->header10.miscFlag = 0;
	this->header10.arraySize = 0;
	this->header10.reserved = 0;
}

void DDSHeader::setWidth(uint w)
{
	this->flags |= DDSD_WIDTH;
	this->width = w;
}

void DDSHeader::setHeight(uint h)
{
	this->flags |= DDSD_HEIGHT;
	this->height = h;
}

void DDSHeader::setDepth(uint d)
{
	this->flags |= DDSD_DEPTH;
	this->height = d;
}

void DDSHeader::setMipmapCount(uint count)
{
	if (count == 0 || count == 1)
	{
		this->flags &= ~DDSD_MIPMAPCOUNT;
		this->mipmapcount = 0;
		
		if (this->caps.caps2 == 0) {
			this->caps.caps1 = DDSCAPS_TEXTURE;
		}
		else {
			this->caps.caps1 = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX;
		}
	}
	else
	{
		this->flags |= DDSD_MIPMAPCOUNT;
		this->mipmapcount = count;

		this->caps.caps1 |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
	}
}

void DDSHeader::setTexture2D()
{
	this->header10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
}

void DDSHeader::setTexture3D()
{
	this->caps.caps2 = DDSCAPS2_VOLUME;
	
	this->header10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE3D;
}

void DDSHeader::setTextureCube()
{
	this->caps.caps1 |= DDSCAPS_COMPLEX;
	this->caps.caps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES;
	
	this->header10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
	this->header10.arraySize = 6;
}

void DDSHeader::setLinearSize(uint size)
{
	this->flags &= ~DDSD_PITCH;
	this->flags |= DDSD_LINEARSIZE;
	this->pitch = size;
}

void DDSHeader::setPitch(uint pitch)
{
	this->flags &= ~DDSD_LINEARSIZE;
	this->flags |= DDSD_PITCH;
	this->pitch = pitch;
}

void DDSHeader::setFourCC(uint8 c0, uint8 c1, uint8 c2, uint8 c3)
{
	// set fourcc pixel format.
	this->pf.flags = DDPF_FOURCC;
	this->pf.fourcc = MAKEFOURCC(c0, c1, c2, c3);
	
	if (this->pf.fourcc == FOURCC_ATI2)
	{
		this->pf.bitcount = FOURCC_A2XY;
	}
	else
	{
		this->pf.bitcount = 0;
	}
	
	this->pf.rmask = 0;
	this->pf.gmask = 0;
	this->pf.bmask = 0;
	this->pf.amask = 0;
}

void DDSHeader::setPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask)
{
	// Make sure the masks are correct.
	nvCheck((rmask & gmask) == 0);
	nvCheck((rmask & bmask) == 0);
	nvCheck((rmask & amask) == 0);
	nvCheck((gmask & bmask) == 0);
	nvCheck((gmask & amask) == 0);
	nvCheck((bmask & amask) == 0);

	this->pf.flags = DDPF_RGB;

	if (amask != 0) {
		this->pf.flags |= DDPF_ALPHAPIXELS;
	}

	if (bitcount == 0)
	{
		// Compute bit count from the masks.
		uint total = rmask | gmask | bmask | amask;
		while(total != 0) {
			bitcount++;
			total >>= 1;
		}
	}

	nvCheck(bitcount > 0 && bitcount <= 32);

	// Align to 8.
	if (bitcount <= 8) bitcount = 8;
	else if (bitcount <= 16) bitcount = 16;
	else if (bitcount <= 24) bitcount = 24;
	else bitcount = 32;

	this->pf.fourcc = 0; //findD3D9Format(bitcount, rmask, gmask, bmask, amask);
	this->pf.bitcount = bitcount;
	this->pf.rmask = rmask;
	this->pf.gmask = gmask;
	this->pf.bmask = bmask;
	this->pf.amask = amask;
}

void DDSHeader::setDX10Format(uint format)
{
	//this->pf.flags = 0;
	this->pf.fourcc = FOURCC_DX10;
	this->header10.dxgiFormat = format;
}

void DDSHeader::setNormalFlag(bool b)
{
	if (b) this->pf.flags |= DDPF_NORMAL;
	else this->pf.flags &= ~DDPF_NORMAL;
}

void DDSHeader::swapBytes()
{
	this->fourcc = POSH_LittleU32(this->fourcc);
	this->size = POSH_LittleU32(this->size);
	this->flags = POSH_LittleU32(this->flags);
	this->height = POSH_LittleU32(this->height);
	this->width = POSH_LittleU32(this->width);
	this->pitch = POSH_LittleU32(this->pitch);
	this->depth = POSH_LittleU32(this->depth);
	this->mipmapcount = POSH_LittleU32(this->mipmapcount);
	
	for(int i = 0; i < 11; i++) {
		this->reserved[i] = POSH_LittleU32(this->reserved[i]);
	}

	this->pf.size = POSH_LittleU32(this->pf.size);
	this->pf.flags = POSH_LittleU32(this->pf.flags);
	this->pf.fourcc = POSH_LittleU32(this->pf.fourcc);
	this->pf.bitcount = POSH_LittleU32(this->pf.bitcount);
	this->pf.rmask = POSH_LittleU32(this->pf.rmask);
	this->pf.gmask = POSH_LittleU32(this->pf.gmask);
	this->pf.bmask = POSH_LittleU32(this->pf.bmask);
	this->pf.amask = POSH_LittleU32(this->pf.amask);
	this->caps.caps1 = POSH_LittleU32(this->caps.caps1);
	this->caps.caps2 = POSH_LittleU32(this->caps.caps2);
	this->caps.caps3 = POSH_LittleU32(this->caps.caps3);
	this->caps.caps4 = POSH_LittleU32(this->caps.caps4);
	this->notused = POSH_LittleU32(this->notused);

	this->header10.dxgiFormat = POSH_LittleU32(this->header10.dxgiFormat);
	this->header10.resourceDimension = POSH_LittleU32(this->header10.resourceDimension);
	this->header10.miscFlag = POSH_LittleU32(this->header10.miscFlag);
	this->header10.arraySize = POSH_LittleU32(this->header10.arraySize);
	this->header10.reserved = POSH_LittleU32(this->header10.reserved);
}

bool DDSHeader::hasDX10Header() const
{
	return this->pf.fourcc == FOURCC_DX10;  // @@ This is according to AMD
	//return this->pf.flags == 0;             // @@ This is according to MS
}



DirectDrawSurface::DirectDrawSurface(const char * name) : stream(new StdInputStream(name))
{
	if (!stream->isError())
	{
		(*stream) << header;
	}
}

DirectDrawSurface::~DirectDrawSurface()
{
	delete stream;
}

bool DirectDrawSurface::isValid() const
{
	if (stream->isError())
	{
		return false;
	}
	
	if (header.fourcc != FOURCC_DDS || header.size != 124)
	{
		return false;
	}
	
	const uint required = (DDSD_WIDTH|DDSD_HEIGHT/*|DDSD_CAPS|DDSD_PIXELFORMAT*/);
	if( (header.flags & required) != required ) {
		return false;
	}
	
	if (header.pf.size != 32) {
		return false;
	}
	
	if( !(header.caps.caps1 & DDSCAPS_TEXTURE) ) {
		return false;
	}
	
	return true;
}

bool DirectDrawSurface::isSupported() const
{
	nvDebugCheck(isValid());
	
	if (header.hasDX10Header())
	{
	}
	else
	{
		if (header.pf.flags & DDPF_FOURCC)
		{
			if (header.pf.fourcc != FOURCC_DXT1 &&
				header.pf.fourcc != FOURCC_DXT2 &&
				header.pf.fourcc != FOURCC_DXT3 &&
				header.pf.fourcc != FOURCC_DXT4 &&
				header.pf.fourcc != FOURCC_DXT5 &&
				header.pf.fourcc != FOURCC_RXGB &&
				header.pf.fourcc != FOURCC_ATI1 &&
				header.pf.fourcc != FOURCC_ATI2)
			{
				// Unknown fourcc code.
				return false;
			}
		}
		else if (header.pf.flags & DDPF_RGB)
		{
			// All RGB formats are supported now.
		}
		else
		{
			return false;
		}
		
		if (isTextureCube() && (header.caps.caps2 & DDSCAPS2_CUBEMAP_ALL_FACES) != DDSCAPS2_CUBEMAP_ALL_FACES)
		{
			// Cubemaps must contain all faces.
			return false;
		}
		
		if (isTexture3D())
		{
			// @@ 3D textures not supported yet.
			return false;
		}
	}
	
	return true;
}


uint DirectDrawSurface::mipmapCount() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_MIPMAPCOUNT) return header.mipmapcount;
	else return 1;
}


uint DirectDrawSurface::width() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_WIDTH) return header.width;
	else return 1;
}

uint DirectDrawSurface::height() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_HEIGHT) return header.height;
	else return 1;
}

uint DirectDrawSurface::depth() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_DEPTH) return header.depth;
	else return 1;
}

bool DirectDrawSurface::isTexture1D() const
{
	nvDebugCheck(isValid());
	if (header.hasDX10Header())
	{
		return header.header10.resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE1D;
	}
	return false;
}

bool DirectDrawSurface::isTexture2D() const
{
	nvDebugCheck(isValid());
	if (header.hasDX10Header())
	{
		return header.header10.resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE2D;
	}
	else
	{
		return !isTexture3D() && !isTextureCube();
	}
}

bool DirectDrawSurface::isTexture3D() const
{
	nvDebugCheck(isValid());
	if (header.hasDX10Header())
	{
		return header.header10.resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D;
	}
	else
	{
		return (header.caps.caps2 & DDSCAPS2_VOLUME) != 0;
	}
}

bool DirectDrawSurface::isTextureCube() const
{
	nvDebugCheck(isValid());
	return (header.caps.caps2 & DDSCAPS2_CUBEMAP) != 0;
}

void DirectDrawSurface::setNormalFlag(bool b)
{
	nvDebugCheck(isValid());
	header.setNormalFlag(b);
}

void DirectDrawSurface::mipmap(Image * img, uint face, uint mipmap)
{
	nvDebugCheck(isValid());
	
	stream->seek(offset(face, mipmap));
	
	uint w = width();
	uint h = height();
	
	// Compute width and height.
	for (uint m = 0; m < mipmap; m++)
	{
		w = max(1U, w / 2);
		h = max(1U, h / 2);
	}
	
	img->allocate(w, h);
	
	if (header.pf.flags & DDPF_RGB) 
	{
		readLinearImage(img);
	}
	else if (header.pf.flags & DDPF_FOURCC)
	{
		readBlockImage(img);
	}
}

void DirectDrawSurface::readLinearImage(Image * img)
{
	nvDebugCheck(stream != NULL);
	nvDebugCheck(img != NULL);
	
	const uint w = img->width();
	const uint h = img->height();

	uint rshift, rsize;
	PixelFormat::maskShiftAndSize(header.pf.rmask, &rshift, &rsize);
	
	uint gshift, gsize;
	PixelFormat::maskShiftAndSize(header.pf.gmask, &gshift, &gsize);
	
	uint bshift, bsize;
	PixelFormat::maskShiftAndSize(header.pf.bmask, &bshift, &bsize);
	
	uint ashift, asize;
	PixelFormat::maskShiftAndSize(header.pf.amask, &ashift, &asize);

	uint byteCount = (header.pf.bitcount + 7) / 8;

	// set image format: RGB or ARGB
	// alpha channel exists if and only if the alpha mask is non-zero
	if (header.pf.amask == 0)
 	{
		img->setFormat(Image::Format_RGB);
	}
	else
	{
		img->setFormat(Image::Format_ARGB);
	}

	// Read linear RGB images.
	for (uint y = 0; y < h; y++)
	{
		for (uint x = 0; x < w; x++)
		{
			uint c = 0;
			stream->serialize(&c, byteCount);

			Color32 pixel(0, 0, 0, 0xFF);
			pixel.r = PixelFormat::convert((c & header.pf.rmask) >> rshift, rsize, 8);
			pixel.g = PixelFormat::convert((c & header.pf.gmask) >> gshift, gsize, 8);
			pixel.b = PixelFormat::convert((c & header.pf.bmask) >> bshift, bsize, 8);
			pixel.a = PixelFormat::convert((c & header.pf.amask) >> ashift, asize, 8);

			img->pixel(x, y) = pixel;
		}
	}
}

void DirectDrawSurface::readBlockImage(Image * img)
{
	nvDebugCheck(stream != NULL);
	nvDebugCheck(img != NULL);

	// set image format: RGB or ARGB
	if (header.pf.fourcc == FOURCC_RXGB ||
		header.pf.fourcc == FOURCC_ATI1 ||
		header.pf.fourcc == FOURCC_ATI2 ||
		header.pf.flags & DDPF_NORMAL)
	{
		img->setFormat(Image::Format_RGB);
	}
	else
	{
		img->setFormat(Image::Format_ARGB);
	}

	const uint w = img->width();
	const uint h = img->height();
	
	const uint bw = (w + 3) / 4;
	const uint bh = (h + 3) / 4;
	
	for (uint by = 0; by < bh; by++)
	{
		for (uint bx = 0; bx < bw; bx++)
		{
			ColorBlock block;
			
			// Read color block.
			readBlock(&block);
			
			// Write color block.
			for (uint y = 0; y < min(4U, h-4*by); y++)
			{
				for (uint x = 0; x < min(4U, w-4*bx); x++)
				{
					img->pixel(4*bx+x, 4*by+y) = block.color(x, y);
				}
			}
		}
	}
}

static Color32 buildNormal(uint8 x, uint8 y)
{
	float nx = 2 * (x / 255.0f) - 1;
	float ny = 2 * (y / 255.0f) - 1;
	float nz = 0.0f;
	if (1 - nx*nx - ny*ny > 0) nz = sqrtf(1 - nx*nx - ny*ny);
	uint8 z = clamp(int(255.0f * (nz + 1) / 2.0f), 0, 255);
	
	return Color32(x, y, z);
}


void DirectDrawSurface::readBlock(ColorBlock * rgba)
{
	nvDebugCheck(stream != NULL);
	nvDebugCheck(rgba != NULL);
	
	if (header.pf.fourcc == FOURCC_DXT1)
	{
		BlockDXT1 block;
		*stream << block;
		block.decodeBlock(rgba);
	}
	else if (header.pf.fourcc == FOURCC_DXT2 ||
	    header.pf.fourcc == FOURCC_DXT3)
	{
		BlockDXT3 block;
		*stream << block;
		block.decodeBlock(rgba);
	}
	else if (header.pf.fourcc == FOURCC_DXT4 ||
	    header.pf.fourcc == FOURCC_DXT5 ||
	    header.pf.fourcc == FOURCC_RXGB)
	{
		BlockDXT5 block;
		*stream << block;
		block.decodeBlock(rgba);
		
		if (header.pf.fourcc == FOURCC_RXGB)
		{
			// Swap R & A.
			for (int i = 0; i < 16; i++)
			{
				Color32 & c = rgba->color(i);
				uint tmp = c.r;
				c.r = c.a;
				c.a = tmp;
			}
		}
	}
	else if (header.pf.fourcc == FOURCC_ATI1)
	{
		BlockATI1 block;
		*stream << block;
		block.decodeBlock(rgba);
	}
	else if (header.pf.fourcc == FOURCC_ATI2)
	{
		BlockATI2 block;
		*stream << block;
		block.decodeBlock(rgba);
	}
	
	// If normal flag set, convert to normal.
	if (header.pf.flags & DDPF_NORMAL)
	{
		if (header.pf.fourcc == FOURCC_ATI2)
		{
			for (int i = 0; i < 16; i++)
			{
				Color32 & c = rgba->color(i);
				c = buildNormal(c.r, c.g);
			}
		}
		else if (header.pf.fourcc == FOURCC_DXT5)
		{
			for (int i = 0; i < 16; i++)
			{
				Color32 & c = rgba->color(i);
				c = buildNormal(c.a, c.g);
			}
		}
	}
}


uint DirectDrawSurface::blockSize() const
{
	switch(header.pf.fourcc)
	{
		case FOURCC_DXT1:
		case FOURCC_ATI1:
			return 8;
		case FOURCC_DXT2:
		case FOURCC_DXT3:
		case FOURCC_DXT4:
		case FOURCC_DXT5:
		case FOURCC_RXGB:
		case FOURCC_ATI2:
			return 16;
	};

	// Not a block image.
	return 0;
}

uint DirectDrawSurface::mipmapSize(uint mipmap) const
{
	uint w = width();
	uint h = height();
	uint d = depth();
	
	for (uint m = 0; m < mipmap; m++)
	{
		w = max(1U, w / 2);
		h = max(1U, h / 2);
		d = max(1U, d / 2);
	}

	if (header.pf.flags & DDPF_FOURCC)
	{
		// @@ How are 3D textures aligned?
		w = (w + 3) / 4;
		h = (h + 3) / 4;
		return blockSize() * w * h;
	}
	else
	{
		nvDebugCheck(header.pf.flags & DDPF_RGB);
		
		// Align pixels to bytes.
		uint byteCount = (header.pf.bitcount + 7) / 8;
		
		// Align pitch to 4 bytes.
		uint pitch = 4 * ((w * byteCount + 3) / 4);
		
		return pitch * h * d;
	}
}

uint DirectDrawSurface::faceSize() const
{
	const uint count = mipmapCount();
	uint size = 0;
	
	for (uint m = 0; m < count; m++)
	{
		size += mipmapSize(m);
	}
	
	return size;
}

uint DirectDrawSurface::offset(const uint face, const uint mipmap)
{
	uint size = 128; // sizeof(DDSHeader);
	
	if (header.hasDX10Header())
	{
		size += 20; // sizeof(DDSHeader10);
	}

	if (face != 0)
	{
		size += face * faceSize();
	}
	
	for (uint m = 0; m < mipmap; m++)
	{
		size += mipmapSize(m);
	}
	
	return size;
}


void DirectDrawSurface::printInfo() const
{
	printf("Flags: 0x%.8X\n", header.flags);
	if (header.flags & DDSD_CAPS) printf("\tDDSD_CAPS\n");
	if (header.flags & DDSD_PIXELFORMAT) printf("\tDDSD_PIXELFORMAT\n");
	if (header.flags & DDSD_WIDTH) printf("\tDDSD_WIDTH\n");
	if (header.flags & DDSD_HEIGHT) printf("\tDDSD_HEIGHT\n");
	if (header.flags & DDSD_DEPTH) printf("\tDDSD_DEPTH\n");
	if (header.flags & DDSD_PITCH) printf("\tDDSD_PITCH\n");
	if (header.flags & DDSD_LINEARSIZE) printf("\tDDSD_LINEARSIZE\n");
	if (header.flags & DDSD_MIPMAPCOUNT) printf("\tDDSD_MIPMAPCOUNT\n");

	printf("Height: %d\n", header.height);
	printf("Width: %d\n", header.width);
	printf("Depth: %d\n", header.depth);
	if (header.flags & DDSD_PITCH) printf("Pitch: %d\n", header.pitch);
	else if (header.flags & DDSD_LINEARSIZE) printf("Linear size: %d\n", header.pitch);
	printf("Mipmap count: %d\n", header.mipmapcount);
	
	printf("Pixel Format:\n");
	printf("\tFlags: 0x%.8X\n", header.pf.flags);
	if (header.pf.flags & DDPF_RGB) printf("\t\tDDPF_RGB\n");
	if (header.pf.flags & DDPF_FOURCC) printf("\t\tDDPF_FOURCC\n");
	if (header.pf.flags & DDPF_ALPHAPIXELS) printf("\t\tDDPF_ALPHAPIXELS\n");
	if (header.pf.flags & DDPF_ALPHA) printf("\t\tDDPF_ALPHA\n");
	if (header.pf.flags & DDPF_PALETTEINDEXED1) printf("\t\tDDPF_PALETTEINDEXED1\n");
	if (header.pf.flags & DDPF_PALETTEINDEXED2) printf("\t\tDDPF_PALETTEINDEXED2\n");
	if (header.pf.flags & DDPF_PALETTEINDEXED4) printf("\t\tDDPF_PALETTEINDEXED4\n");
	if (header.pf.flags & DDPF_PALETTEINDEXED8) printf("\t\tDDPF_PALETTEINDEXED8\n");
	if (header.pf.flags & DDPF_ALPHAPREMULT) printf("\t\tDDPF_ALPHAPREMULT\n");
	if (header.pf.flags & DDPF_NORMAL) printf("\t\tDDPF_NORMAL\n");
	
	printf("\tFourCC: '%c%c%c%c'\n",
		((header.pf.fourcc >> 0) & 0xFF),
		((header.pf.fourcc >> 8) & 0xFF),
		((header.pf.fourcc >> 16) & 0xFF),
		((header.pf.fourcc >> 24) & 0xFF));
	if ((header.pf.fourcc & DDPF_FOURCC) && (header.pf.bitcount != 0))
	{
		printf("\tSwizzle: '%c%c%c%c'\n", 
			(header.pf.bitcount >> 0) & 0xFF,
			(header.pf.bitcount >> 8) & 0xFF,
			(header.pf.bitcount >> 16) & 0xFF,
			(header.pf.bitcount >> 24) & 0xFF);
	}
	else
	{
		printf("\tBit count: %d\n", header.pf.bitcount);
	}
	printf("\tRed mask: 0x%.8X\n", header.pf.rmask);
	printf("\tGreen mask: 0x%.8X\n", header.pf.gmask);
	printf("\tBlue mask: 0x%.8X\n", header.pf.bmask);
	printf("\tAlpha mask: 0x%.8X\n", header.pf.amask);

	printf("Caps:\n");
	printf("\tCaps 1: 0x%.8X\n", header.caps.caps1);
	if (header.caps.caps1 & DDSCAPS_COMPLEX) printf("\t\tDDSCAPS_COMPLEX\n");
	if (header.caps.caps1 & DDSCAPS_TEXTURE) printf("\t\tDDSCAPS_TEXTURE\n");
	if (header.caps.caps1 & DDSCAPS_MIPMAP) printf("\t\tDDSCAPS_MIPMAP\n");

	printf("\tCaps 2: 0x%.8X\n", header.caps.caps2);
	if (header.caps.caps2 & DDSCAPS2_VOLUME) printf("\t\tDDSCAPS2_VOLUME\n");
	else if (header.caps.caps2 & DDSCAPS2_CUBEMAP)
	{
		printf("\t\tDDSCAPS2_CUBEMAP\n");
		if ((header.caps.caps2 & DDSCAPS2_CUBEMAP_ALL_FACES) == DDSCAPS2_CUBEMAP_ALL_FACES) printf("\t\tDDSCAPS2_CUBEMAP_ALL_FACES\n");
		else {
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEX) printf("\t\tDDSCAPS2_CUBEMAP_POSITIVEX\n");
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) printf("\t\tDDSCAPS2_CUBEMAP_NEGATIVEX\n");
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEY) printf("\t\tDDSCAPS2_CUBEMAP_POSITIVEY\n");
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) printf("\t\tDDSCAPS2_CUBEMAP_NEGATIVEY\n");
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) printf("\t\tDDSCAPS2_CUBEMAP_POSITIVEZ\n");
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) printf("\t\tDDSCAPS2_CUBEMAP_NEGATIVEZ\n");
		}
	}

	printf("\tCaps 3: 0x%.8X\n", header.caps.caps3);
	printf("\tCaps 4: 0x%.8X\n", header.caps.caps4);

	if (header.hasDX10Header())
	{
		printf("DX10 Header:\n");
		printf("\tDXGI Format: %u (%s)\n", header.header10.dxgiFormat, getDxgiFormatString((DXGI_FORMAT)header.header10.dxgiFormat));
		printf("\tResource dimension: %u (%s)\n", header.header10.resourceDimension, getD3d10ResourceDimensionString((D3D10_RESOURCE_DIMENSION)header.header10.resourceDimension));
		printf("\tMisc flag: %u\n", header.header10.miscFlag);
		printf("\tArray size: %u\n", header.header10.arraySize);
	}
	
	if (header.reserved[9] == MAKEFOURCC('N', 'V', 'T', 'T'))
	{
		int major = (header.reserved[10] >> 16) & 0xFF;
		int minor = (header.reserved[10] >> 8) & 0xFF;
		int revision= header.reserved[10] & 0xFF;
		
		printf("Version:\n");
		printf("\tNVIDIA Texture Tools %d.%d.%d\n", major, minor, revision);
	}
}

