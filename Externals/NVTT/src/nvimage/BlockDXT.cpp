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

#include <nvcore/Stream.h>

#include "ColorBlock.h"
#include "BlockDXT.h"

using namespace nv;


/*----------------------------------------------------------------------------
	BlockDXT1
----------------------------------------------------------------------------*/

uint BlockDXT1::evaluatePalette(Color32 color_array[4]) const
{
	// Does bit expansion before interpolation.
	color_array[0].b = (col0.b << 3) | (col0.b >> 2);
	color_array[0].g = (col0.g << 2) | (col0.g >> 4);
	color_array[0].r = (col0.r << 3) | (col0.r >> 2);
	color_array[0].a = 0xFF;
	
	// @@ Same as above, but faster?
//	Color32 c;
//	c.u = ((col0.u << 3) & 0xf8) | ((col0.u << 5) & 0xfc00) | ((col0.u << 8) & 0xf80000);
//	c.u |= (c.u >> 5) & 0x070007;
//	c.u |= (c.u >> 6) & 0x000300;
//	color_array[0].u = c.u;
	
	color_array[1].r = (col1.r << 3) | (col1.r >> 2);
	color_array[1].g = (col1.g << 2) | (col1.g >> 4);
	color_array[1].b = (col1.b << 3) | (col1.b >> 2);
	color_array[1].a = 0xFF;
	
	// @@ Same as above, but faster?
//	c.u = ((col1.u << 3) & 0xf8) | ((col1.u << 5) & 0xfc00) | ((col1.u << 8) & 0xf80000);
//	c.u |= (c.u >> 5) & 0x070007;
//	c.u |= (c.u >> 6) & 0x000300;
//	color_array[1].u = c.u;
	
	if( col0.u > col1.u ) {
		// Four-color block: derive the other two colors.
		color_array[2].r = (2 * color_array[0].r + color_array[1].r) / 3;
		color_array[2].g = (2 * color_array[0].g + color_array[1].g) / 3;
		color_array[2].b = (2 * color_array[0].b + color_array[1].b) / 3;
		color_array[2].a = 0xFF;
		
		color_array[3].r = (2 * color_array[1].r + color_array[0].r) / 3;
		color_array[3].g = (2 * color_array[1].g + color_array[0].g) / 3;
		color_array[3].b = (2 * color_array[1].b + color_array[0].b) / 3;
		color_array[3].a = 0xFF;
		
		return 4;
	}
	else {
		// Three-color block: derive the other color.
		color_array[2].r = (color_array[0].r + color_array[1].r) / 2;
		color_array[2].g = (color_array[0].g + color_array[1].g) / 2;
		color_array[2].b = (color_array[0].b + color_array[1].b) / 2;
		color_array[2].a = 0xFF;
		
		// Set all components to 0 to match DXT specs.
		color_array[3].r = 0x00; // color_array[2].r;
		color_array[3].g = 0x00; // color_array[2].g;
		color_array[3].b = 0x00; // color_array[2].b;
		color_array[3].a = 0x00;
		
		return 3;
	}
}

// Evaluate palette assuming 3 color block.
void BlockDXT1::evaluatePalette3(Color32 color_array[4]) const
{
	color_array[0].b = (col0.b << 3) | (col0.b >> 2);
	color_array[0].g = (col0.g << 2) | (col0.g >> 4);
	color_array[0].r = (col0.r << 3) | (col0.r >> 2);
	color_array[0].a = 0xFF;
	
	color_array[1].r = (col1.r << 3) | (col1.r >> 2);
	color_array[1].g = (col1.g << 2) | (col1.g >> 4);
	color_array[1].b = (col1.b << 3) | (col1.b >> 2);
	color_array[1].a = 0xFF;
	
	// Three-color block: derive the other color.
	color_array[2].r = (color_array[0].r + color_array[1].r) / 2;
	color_array[2].g = (color_array[0].g + color_array[1].g) / 2;
	color_array[2].b = (color_array[0].b + color_array[1].b) / 2;
	color_array[2].a = 0xFF;
		
	// Set all components to 0 to match DXT specs.
	color_array[3].r = 0x00; // color_array[2].r;
	color_array[3].g = 0x00; // color_array[2].g;
	color_array[3].b = 0x00; // color_array[2].b;
	color_array[3].a = 0x00;
}

// Evaluate palette assuming 4 color block.
void BlockDXT1::evaluatePalette4(Color32 color_array[4]) const
{
	color_array[0].b = (col0.b << 3) | (col0.b >> 2);
	color_array[0].g = (col0.g << 2) | (col0.g >> 4);
	color_array[0].r = (col0.r << 3) | (col0.r >> 2);
	color_array[0].a = 0xFF;
	
	color_array[1].r = (col1.r << 3) | (col1.r >> 2);
	color_array[1].g = (col1.g << 2) | (col1.g >> 4);
	color_array[1].b = (col1.b << 3) | (col1.b >> 2);
	color_array[1].a = 0xFF;
	
	// Four-color block: derive the other two colors.
	color_array[2].r = (2 * color_array[0].r + color_array[1].r) / 3;
	color_array[2].g = (2 * color_array[0].g + color_array[1].g) / 3;
	color_array[2].b = (2 * color_array[0].b + color_array[1].b) / 3;
	color_array[2].a = 0xFF;
		
	color_array[3].r = (2 * color_array[1].r + color_array[0].r) / 3;
	color_array[3].g = (2 * color_array[1].g + color_array[0].g) / 3;
	color_array[3].b = (2 * color_array[1].b + color_array[0].b) / 3;
	color_array[3].a = 0xFF;
}


/* Jason Dorie's code.
// ----------------------------------------------------------------------------
// Build palette for a 3 color + traparent black block
// ----------------------------------------------------------------------------
void DXTCGen::BuildCodes3(cbVector *pVects, cbVector &v1, cbVector &v2)
{
	//pVects[0] = v1;
	//pVects[2] = v2;
	//pVects[1][0] = v1[0];
	//pVects[1][1] = (BYTE)( ((long)v1[1] + (long)v2[1]) / 2 );
	//pVects[1][2] = (BYTE)( ((long)v1[2] + (long)v2[2]) / 2 );
	//pVects[1][3] = (BYTE)( ((long)v1[3] + (long)v2[3]) / 2 );

	__asm {
		mov			ecx, dword ptr pVects
		mov			eax, dword ptr v1
		mov			ebx, dword ptr v2

		movd		mm0, [eax]
		movd		mm1, [ebx]
		pxor		mm2, mm2
		nop

		movd		[ecx], mm0
		movd		[ecx+8], mm1

		punpcklbw	mm0, mm2
		punpcklbw	mm1, mm2

		paddw		mm0, mm1
		psrlw		mm0, 1

		packuswb	mm0, mm0
		movd		[ecx+4], mm0
	}
	// *(long *)&pVects[1] = r1;
}

__int64 ScaleOneThird = 0x5500550055005500;

// ----------------------------------------------------------------------------
// Build palette for a 4 color block
// ----------------------------------------------------------------------------
void DXTCGen::BuildCodes4(cbVector *pVects, cbVector &v1, cbVector &v2)
{
// 	pVects[0] = v1;
// 	pVects[3] = v2;
// 
// 	pVects[1][0] = v1[0];
// 	pVects[1][1] = (BYTE)( ((long)v1[1] * 2 + (long)v2[1]) / 3 );
// 	pVects[1][2] = (BYTE)( ((long)v1[2] * 2 + (long)v2[2]) / 3 );
// 	pVects[1][3] = (BYTE)( ((long)v1[3] * 2 + (long)v2[3]) / 3 );
// 
// 	pVects[2][0] = v1[0];
// 	pVects[2][1] = (BYTE)( ((long)v2[1] * 2 + (long)v1[1]) / 3 );
// 	pVects[2][2] = (BYTE)( ((long)v2[2] * 2 + (long)v1[2]) / 3 );
// 	pVects[2][3] = (BYTE)( ((long)v2[3] * 2 + (long)v1[3]) / 3 );

	__asm {
		mov			ecx, dword ptr pVects
		mov			eax, dword ptr v1
		mov			ebx, dword ptr v2

		movd		mm0, [eax]
		movd		mm1, [ebx]

		pxor		mm2, mm2
		movd		[ecx], mm0
		movd		[ecx+12], mm1

		punpcklbw	mm0, mm2
		punpcklbw	mm1, mm2
		movq		mm3, mm0		// mm3 = v0

		paddw		mm0, mm1		// mm0 = v0 + v1
		paddw		mm3, mm3		// mm3 = v0*2

		paddw		mm0, mm1		// mm0 = v0 + v1*2
		paddw		mm1, mm3		// mm1 = v0*2 + v1

		pmulhw		mm0, ScaleOneThird
		pmulhw		mm1, ScaleOneThird
		packuswb	mm1, mm0

		movq		[ecx+4], mm1
	}
}
*/

void BlockDXT1::decodeBlock(ColorBlock * block) const
{
	nvDebugCheck(block != NULL);
	
	// Decode color block.
	Color32 color_array[4];
	evaluatePalette(color_array);
	
	// Write color block.
	for( uint j = 0; j < 4; j++ ) {
		for( uint i = 0; i < 4; i++ ) {
			uint idx = (row[j] >> (2 * i)) & 3;
			block->color(i, j) = color_array[idx];
		}
	}	
}

void BlockDXT1::setIndices(int * idx)
{
	indices = 0;
	for(uint i = 0; i < 16; i++) {
		indices |= (idx[i] & 3) << (2 * i);
	}
}


/// Flip DXT1 block vertically.
inline void BlockDXT1::flip4()
{
	swap(row[0], row[3]);
	swap(row[1], row[2]);
}

/// Flip half DXT1 block vertically.
inline void BlockDXT1::flip2()
{
	swap(row[0], row[1]);
}


/*----------------------------------------------------------------------------
	BlockDXT3
----------------------------------------------------------------------------*/

void BlockDXT3::decodeBlock(ColorBlock * block) const
{
	nvDebugCheck(block != NULL);
	
	// Decode color.
	color.decodeBlock(block);
	
	// Decode alpha.
	alpha.decodeBlock(block);
}

void AlphaBlockDXT3::decodeBlock(ColorBlock * block) const
{
	nvDebugCheck(block != NULL);

	block->color(0x0).a = (alpha0 << 4) | alpha0;
	block->color(0x1).a = (alpha1 << 4) | alpha1;
	block->color(0x2).a = (alpha2 << 4) | alpha2;
	block->color(0x3).a = (alpha3 << 4) | alpha3;
	block->color(0x4).a = (alpha4 << 4) | alpha4;
	block->color(0x5).a = (alpha5 << 4) | alpha5;
	block->color(0x6).a = (alpha6 << 4) | alpha6;
	block->color(0x7).a = (alpha7 << 4) | alpha7;
	block->color(0x8).a = (alpha8 << 4) | alpha8;
	block->color(0x9).a = (alpha9 << 4) | alpha9;
	block->color(0xA).a = (alphaA << 4) | alphaA;
	block->color(0xB).a = (alphaB << 4) | alphaB;
	block->color(0xC).a = (alphaC << 4) | alphaC;
	block->color(0xD).a = (alphaD << 4) | alphaD;
	block->color(0xE).a = (alphaE << 4) | alphaE;
	block->color(0xF).a = (alphaF << 4) | alphaF;
}

/// Flip DXT3 alpha block vertically.
void AlphaBlockDXT3::flip4()
{
	swap(row[0], row[3]);
	swap(row[1], row[2]);
}

/// Flip half DXT3 alpha block vertically.
void AlphaBlockDXT3::flip2()
{
	swap(row[0], row[1]);
}

/// Flip DXT3 block vertically.
void BlockDXT3::flip4()
{
	alpha.flip4();
	color.flip4();
}

/// Flip half DXT3 block vertically.
void BlockDXT3::flip2()
{
	alpha.flip2();
	color.flip2();
}


/*----------------------------------------------------------------------------
	BlockDXT5
----------------------------------------------------------------------------*/

void AlphaBlockDXT5::evaluatePalette(uint8 alpha[8]) const
{
	if (alpha0 > alpha1) {
		evaluatePalette8(alpha);
	}
	else {
		evaluatePalette6(alpha);
	}
}

void AlphaBlockDXT5::evaluatePalette8(uint8 alpha[8]) const
{
	// 8-alpha block:  derive the other six alphas.
	// Bit code 000 = alpha0, 001 = alpha1, others are interpolated.
	alpha[0] = alpha0;
	alpha[1] = alpha1;
	alpha[2] = (6 * alpha[0] + 1 * alpha[1]) / 7;	// bit code 010
	alpha[3] = (5 * alpha[0] + 2 * alpha[1]) / 7;	// bit code 011
	alpha[4] = (4 * alpha[0] + 3 * alpha[1]) / 7;	// bit code 100
	alpha[5] = (3 * alpha[0] + 4 * alpha[1]) / 7;	// bit code 101
	alpha[6] = (2 * alpha[0] + 5 * alpha[1]) / 7;	// bit code 110
	alpha[7] = (1 * alpha[0] + 6 * alpha[1]) / 7;	// bit code 111
}

void AlphaBlockDXT5::evaluatePalette6(uint8 alpha[8]) const
{
	// 6-alpha block.
	// Bit code 000 = alpha0, 001 = alpha1, others are interpolated.
	alpha[0] = alpha0;
	alpha[1] = alpha1;
	alpha[2] = (4 * alpha[0] + 1 * alpha[1]) / 5;	// Bit code 010
	alpha[3] = (3 * alpha[0] + 2 * alpha[1]) / 5;	// Bit code 011
	alpha[4] = (2 * alpha[0] + 3 * alpha[1]) / 5;	// Bit code 100
	alpha[5] = (1 * alpha[0] + 4 * alpha[1]) / 5;	// Bit code 101
	alpha[6] = 0x00;							// Bit code 110
	alpha[7] = 0xFF;							// Bit code 111
}

void AlphaBlockDXT5::indices(uint8 index_array[16]) const
{
	index_array[0x0] = bits0;
	index_array[0x1] = bits1;
	index_array[0x2] = bits2;
	index_array[0x3] = bits3;
	index_array[0x4] = bits4;
	index_array[0x5] = bits5;
	index_array[0x6] = bits6;
	index_array[0x7] = bits7;
	index_array[0x8] = bits8;
	index_array[0x9] = bits9;
	index_array[0xA] = bitsA;
	index_array[0xB] = bitsB;
	index_array[0xC] = bitsC;
	index_array[0xD] = bitsD;
	index_array[0xE] = bitsE;
	index_array[0xF] = bitsF;
}

uint AlphaBlockDXT5::index(uint index) const
{
	nvDebugCheck(index < 16);

	int offset = (3 * index + 16);
	return uint((this->u >> offset) & 0x7);
}

void AlphaBlockDXT5::setIndex(uint index, uint value)
{
	nvDebugCheck(index < 16);
	nvDebugCheck(value < 8);

	int offset = (3 * index + 16);
	uint64 mask = uint64(0x7) << offset;
	this->u = (this->u & ~mask) | (uint64(value) << offset);
}

void AlphaBlockDXT5::decodeBlock(ColorBlock * block) const
{
	nvDebugCheck(block != NULL);
	
	uint8 alpha_array[8];
	evaluatePalette(alpha_array);
	
	uint8 index_array[16];
	indices(index_array);
	
	for(uint i = 0; i < 16; i++) {
		block->color(i).a = alpha_array[index_array[i]];
	}
}

void AlphaBlockDXT5::flip4()
{
	uint64 * b = (uint64 *)this;
	
	// @@ The masks might have to be byte swapped.
	uint64 tmp = (*b & POSH_U64(0x000000000000FFFF));
	tmp |= (*b & POSH_U64(0x000000000FFF0000)) << 36;
	tmp |= (*b & POSH_U64(0x000000FFF0000000)) << 12;
	tmp |= (*b & POSH_U64(0x000FFF0000000000)) >> 12;
	tmp |= (*b & POSH_U64(0xFFF0000000000000)) >> 36;
	
	*b = tmp;
}

void AlphaBlockDXT5::flip2()
{
	uint * b = (uint *)this;
	
	// @@ The masks might have to be byte swapped.
	uint tmp = (*b & 0xFF000000);
	tmp |=  (*b & 0x00000FFF) << 12;
	tmp |= (*b & 0x00FFF000) >> 12;
	
	*b = tmp;
}

void BlockDXT5::decodeBlock(ColorBlock * block) const
{
	nvDebugCheck(block != NULL);
	
	// Decode color.
	color.decodeBlock(block);
	
	// Decode alpha.
	alpha.decodeBlock(block);

}

/// Flip DXT5 block vertically.
void BlockDXT5::flip4()
{
	alpha.flip4();
	color.flip4();
}

/// Flip half DXT5 block vertically.
void BlockDXT5::flip2()
{
	alpha.flip2();
	color.flip2();
}


/// Decode ATI1 block.
void BlockATI1::decodeBlock(ColorBlock * block) const
{
	uint8 alpha_array[8];
	alpha.evaluatePalette(alpha_array);
	
	uint8 index_array[16];
	alpha.indices(index_array);
	
	for(uint i = 0; i < 16; i++) {
		Color32 & c = block->color(i);
		c.b = c.g = c.r = alpha_array[index_array[i]];
		c.a = 255;
	}
}

/// Flip ATI1 block vertically.
void BlockATI1::flip4()
{
	alpha.flip4();
}

/// Flip half ATI1 block vertically.
void BlockATI1::flip2()
{
	alpha.flip2();
}


/// Decode ATI2 block.
void BlockATI2::decodeBlock(ColorBlock * block) const
{
	uint8 alpha_array[8];
	uint8 index_array[16];
	
	x.evaluatePalette(alpha_array);
	x.indices(index_array);
	
	for(uint i = 0; i < 16; i++) {
		Color32 & c = block->color(i);
		c.r = alpha_array[index_array[i]];
	}

	y.evaluatePalette(alpha_array);
	y.indices(index_array);
	
	for(uint i = 0; i < 16; i++) {
		Color32 & c = block->color(i);
		c.g = alpha_array[index_array[i]];
		c.b = 0;
		c.a = 255;
	}
}

/// Flip ATI2 block vertically.
void BlockATI2::flip4()
{
	x.flip4();
	y.flip4();
}

/// Flip half ATI2 block vertically.
void BlockATI2::flip2()
{
	x.flip2();
	y.flip2();
}


void BlockCTX1::evaluatePalette(Color32 color_array[4]) const
{
	// Does bit expansion before interpolation.
	color_array[0].b = 0x00;
	color_array[0].g = col0[1];
	color_array[0].r = col0[0];
	color_array[0].a = 0xFF;
	
	color_array[1].r = 0x00;
	color_array[1].g = col0[1];
	color_array[1].b = col1[0];
	color_array[1].a = 0xFF;
	
	color_array[2].r = 0x00;
	color_array[2].g = (2 * color_array[0].g + color_array[1].g) / 3;
	color_array[2].b = (2 * color_array[0].b + color_array[1].b) / 3;
	color_array[2].a = 0xFF;
		
	color_array[3].r = 0x00;
	color_array[3].g = (2 * color_array[1].g + color_array[0].g) / 3;
	color_array[3].b = (2 * color_array[1].b + color_array[0].b) / 3;
	color_array[3].a = 0xFF;
}

void BlockCTX1::decodeBlock(ColorBlock * block) const
{
	nvDebugCheck(block != NULL);
	
	// Decode color block.
	Color32 color_array[4];
	evaluatePalette(color_array);
	
	// Write color block.
	for( uint j = 0; j < 4; j++ ) {
		for( uint i = 0; i < 4; i++ ) {
			uint idx = (row[j] >> (2 * i)) & 3;
			block->color(i, j) = color_array[idx];
		}
	}	
}

void BlockCTX1::setIndices(int * idx)
{
	indices = 0;
	for(uint i = 0; i < 16; i++) {
		indices |= (idx[i] & 3) << (2 * i);
	}
}


/// Flip CTX1 block vertically.
inline void BlockCTX1::flip4()
{
	swap(row[0], row[3]);
	swap(row[1], row[2]);
}

/// Flip half CTX1 block vertically.
inline void BlockCTX1::flip2()
{
	swap(row[0], row[1]);
}




Stream & nv::operator<<(Stream & stream, BlockDXT1 & block)
{
	stream << block.col0.u << block.col1.u;
	stream.serialize(&block.indices, sizeof(block.indices));
	return stream;
}

Stream & nv::operator<<(Stream & stream, AlphaBlockDXT3 & block)
{
	stream.serialize(&block, sizeof(block));
	return stream;
}

Stream & nv::operator<<(Stream & stream, BlockDXT3 & block)
{
	return stream << block.alpha << block.color;
}

Stream & nv::operator<<(Stream & stream, AlphaBlockDXT5 & block)
{
	stream.serialize(&block, sizeof(block));
	return stream;
}

Stream & nv::operator<<(Stream & stream, BlockDXT5 & block)
{
	return stream << block.alpha << block.color;
}

Stream & nv::operator<<(Stream & stream, BlockATI1 & block)
{
	return stream << block.alpha;
}

Stream & nv::operator<<(Stream & stream, BlockATI2 & block)
{
	return stream << block.x << block.y;
}

Stream & nv::operator<<(Stream & stream, BlockCTX1 & block)
{
	stream.serialize(&block, sizeof(block));
	return stream;
}

