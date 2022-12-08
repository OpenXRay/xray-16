/*********************************************************************NVMH2****
Path:  C:\Dev\devrel\Nv_sdk_4\Direct3D\Decompress_DXTC
File:  Image_DXTC.cpp

Copyright (C) 1999, 2000 NVIDIA Corporation
This file is provided without support, instruction, or implied warranty of any
kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
not liable under any circumstances for any damages or loss whatsoever arising
from the use or inability to use this file or items derived from it.

Comments:

******************************************************************************/

#include "stdafx.h"

#include "Image_DXTC.h"
#include <ddraw.h>

#define LOW_5 0x001F;
#define MID_6 0x07E0;
#define HIGH_5 0xF800;
#define MID_555 0x03E0;
#define HI_555 0x7C00;

// should be in ddraw.h
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)\
    ((u32)(u8)(ch0) | ((u32)(u8)(ch1) << 8) | ((u32)(u8)(ch2) << 16) | ((u32)(u8)(ch3) << 24))
#endif // defined(MAKEFOURCC)

// Defined in this module:
u16 GetNumberOfBits(u32 dwMask);

Image_DXTC::Image_DXTC()
{
    m_pCompBytes = NULL;
    m_pDecompBytes = NULL;
}

Image_DXTC::~Image_DXTC()
{
    if (m_pCompBytes != NULL)
    {
        free(m_pCompBytes);
        m_pCompBytes = NULL;
    }
    if (m_pDecompBytes != NULL)
    {
        free(m_pDecompBytes);
        m_pDecompBytes = NULL;
    }
}

void Image_DXTC::SaveAsRaw()
{
    // save decompressed bits
    FILE* pf = fopen("decom.raw", "wb");
    VERIFY(pf);
    // writes only 32 bit format.
    fwrite(m_pDecompBytes, m_nHeight * m_nWidth * 4, sizeof(u8), pf);
    fclose(pf);
    pf = NULL;
}

bool Image_DXTC::LoadFromFile(LPCSTR filename)
{
    if (m_pCompBytes != NULL)
    {
        free(m_pCompBytes);
        m_pCompBytes = NULL;
    }
    // only understands .dds files for now
    // return true if success
    char* exts[] = {".DDS"};
    int next = 1;
    char fileupper[256];
    strcpy_s(fileupper, filename);
    xr_strupr(fileupper);
    // TRACE( "\n" );
    // TRACE( "\n" );
    bool knownformat = false;
    for (int i = 0; i < next; i++)
    {
        char* found = strstr(fileupper, exts[0]);
        if (found != NULL)
        {
            knownformat = true;
            break;
        }
    }
    if (knownformat == false)
    {
        // TRACE("Unknown file format encountered!  [%s]\n", filename );
        return false;
    }
    // TRACE("\n\nLoading file [%s]\n", filename );
    FILE* file = fopen(filename, "rb");
    if (file == NULL)
    {
        // TRACE("Can't open file for reading! [%s]\n", filename );
        return false;
    }
    // start reading the file
    // from Microsoft's mssdk D3DIM example "Compress"
    DDS_HEADER ddsd;
    u32 dwMagic;
    // Read magic number
    fread(&dwMagic, sizeof(u32), 1, file);
    if (dwMagic != MAKEFOURCC('D', 'D', 'S', ' '))
    {
        fclose(file);
        return false;
    }
    // Read the surface description
    fread(&ddsd, sizeof(DDS_HEADER), 1, file);
    // Does texture have mipmaps?
    m_bMipTexture = ddsd.dwMipMapCount > 0;
    // Clear unwanted flags
    // Can't do this!!!  surface not re-created here
    //    ddsd.dwFlags &= (~DDSD_PITCH);
    //    ddsd.dwFlags &= (~DDSD_LINEARSIZE);
    // Is it DXTC ?
    // I sure hope pixelformat is valid!
    DecodePixelFormat(m_strFormat, &ddsd.ddspf);
    if (m_CompFormat == PF_DXT1 || m_CompFormat == PF_DXT2 || m_CompFormat == PF_DXT3 || m_CompFormat == PF_DXT4 ||
    m_CompFormat == PF_DXT5)
    {
        // TRACE("Yay, a recognized format!\n\n");
    }
    else
    {
        // TRACE("Format is %s.  Not loading!\n", m_strFormat );
        return false;
    }
    // TRACE("ddsd.dwLinearSize:     %d\n", ddsd.dwLinearSize);
    // TRACE("ddsd.dwHeight:         %d\n", ddsd.dwHeight);
    // TRACE("ddsd.dwWidth:          %d\n", ddsd.dwWidth);
    // TRACE("w * h                  %d\n", ddsd.dwWidth * ddsd.dwHeight);
    // Store the return copy of this surfacedesc
    m_DDSD = ddsd;
    m_nHeight = ddsd.dwHeight;
    m_nWidth = ddsd.dwWidth;
    // Read only first mip level for now:
    if (ddsd.dwHeaderFlags & DDSD_LINEARSIZE)
    {
        // TRACE("dwFlags  has DDSD_LINEARSIZE\n");
        m_pCompBytes = (u8*)calloc(ddsd.dwPitchOrLinearSize, sizeof(u8));
        if (m_pCompBytes == NULL)
        {
            // TRACE("Can't allocate m_pCompBytes on file read!\n");
            return (false);
        }
        fread(m_pCompBytes, ddsd.dwPitchOrLinearSize, 1, file);
    }
    else
    {
        // TRACE("dwFlags  file doesn't have linearsize set\n");
        u32 dwBytesPerRow = ddsd.dwWidth * ddsd.ddspf.dwRGBBitCount / 8;
        m_pCompBytes = (u8*)calloc(ddsd.dwPitchOrLinearSize * ddsd.dwHeight, sizeof(u8));
        m_nCompSize = ddsd.dwPitchOrLinearSize * ddsd.dwHeight;
        m_nCompLineSz = dwBytesPerRow;
        if (m_pCompBytes == NULL)
        {
            // TRACE("Can't allocate m_pCompBytes on file read!\n");
            return false;
        }
        u8* pDest = m_pCompBytes;
        for (u32 yp = 0; yp < ddsd.dwHeight; yp++)
        {
            fread(pDest, dwBytesPerRow, 1, file);
            pDest += ddsd.dwPitchOrLinearSize;
        }
    }
    // done reading file
    fclose(file);
    file = NULL;
    return true;
}

void Image_DXTC::AllocateDecompBytes()
{
    if (m_pDecompBytes != NULL)
    {
        free(m_pDecompBytes);
        m_pDecompBytes = NULL;
    }
    // Allocate for 32 bit surface:
    m_pDecompBytes = (u8*)calloc(m_DDSD.dwWidth * m_DDSD.dwHeight * 4, sizeof(u8));
    if (m_pDecompBytes == NULL)
    {
        // TRACE("Error allocating decompressed byte storage\n");
    }
}

void Image_DXTC::Decompress()
{
    VERIFY(m_pCompBytes);
    AllocateDecompBytes();
    VERIFY(m_pDecompBytes); // must already have allocated memory
    switch (m_CompFormat)
    {
    case PF_DXT1:
        // TRACE( "Decompressing image format:  DXT1\n" );
        DecompressDXT1();
        break;
    case PF_DXT2:
        // TRACE( "Decompressing image format:  DXT2\n" );
        DecompressDXT2();
        break;
    case PF_DXT3:
        // TRACE( "Decompressing image format:  DXT3\n" );
        DecompressDXT3();
        break;
    case PF_DXT4:
        // TRACE( "Decompressing image format:  DXT4\n" );
        DecompressDXT4();
        break;
    case PF_DXT5:
        // TRACE( "Decompressing image format:  DXT5\n" );
        DecompressDXT5();
        break;
    case PF_UNKNOWN: break;
    }
    //. swap R<->B channels
    for (int y = 0; y < m_nHeight; y++)
    {
        for (int x = 0; x < m_nWidth; x++)
        {
            u8* ptr = m_pDecompBytes + (y * m_nWidth + x) * 4;
            std::swap(ptr[0], ptr[2]);
        }
    }
}

struct DXTColBlock
{
    u16 col0;
    u16 col1;
    // no bit fields - use bytes
    u8 row[4];
};

struct DXTAlphaBlockExplicit
{
    u16 row[4];
};

struct DXTAlphaBlock3BitLinear
{
    u8 alpha0;
    u8 alpha1;
    u8 stuff[6];
};

// use cast to struct instead of RGBA_MAKE as struct is  much
struct Color8888
{
    u8 r; // change the order of names to change the
    u8 g; //  order of the output ARGB or BGRA, etc...
    u8 b; //  Last one is MSB, 1st is LSB.
    u8 a;
};

struct Color565
{
    unsigned nBlue : 5; // order of names changes
    unsigned nGreen : 6; //  byte order of output to 32 bit
    unsigned nRed : 5;
};

inline void GetColorBlockColors(
DXTColBlock* pBlock, Color8888* col_0, Color8888* col_1, Color8888* col_2, Color8888* col_3, u16& wrd)
{
    // There are 4 methods to use - see the Time_ functions.
    // 1st = shift = does normal approach per byte for color comps
    // 2nd = use freak variable bit field color565 for component extraction
    // 3rd = use super-freak DWORD adds BEFORE shifting the color components
    //  This lets you do only 1 add per color instead of 3 BYTE adds and
    //  might be faster
    // Call RunTimingSession() to run each of them & output result to txt file
    // freak variable bit structure method
    // normal math
    // This method is fastest
    Color565* pCol;
    pCol = (Color565*)&pBlock->col0;
    col_0->a = 0xff;
    col_0->r = pCol->nRed;
    col_0->r <<= 3; // shift to full precision
    col_0->g = pCol->nGreen;
    col_0->g <<= 2;
    col_0->b = pCol->nBlue;
    col_0->b <<= 3;
    pCol = (Color565*)&pBlock->col1;
    col_1->a = 0xff;
    col_1->r = pCol->nRed;
    col_1->r <<= 3; // shift to full precision
    col_1->g = pCol->nGreen;
    col_1->g <<= 2;
    col_1->b = pCol->nBlue;
    col_1->b <<= 3;
    if (pBlock->col0 > pBlock->col1)
    {
        // Four-color block: derive the other two colors.
        // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
        // These two bit codes correspond to the 2-bit fields
        // stored in the 64-bit block.
        wrd = ((u16)col_0->r * 2 + (u16)col_1->r) / 3;
        // no +1 for rounding
        // as bits have been shifted to 888
        col_2->r = (u8)wrd;
        wrd = ((u16)col_0->g * 2 + (u16)col_1->g) / 3;
        col_2->g = (u8)wrd;
        wrd = ((u16)col_0->b * 2 + (u16)col_1->b) / 3;
        col_2->b = (u8)wrd;
        col_2->a = 0xff;
        wrd = ((u16)col_0->r + (u16)col_1->r * 2) / 3;
        col_3->r = (u8)wrd;
        wrd = ((u16)col_0->g + (u16)col_1->g * 2) / 3;
        col_3->g = (u8)wrd;
        wrd = ((u16)col_0->b + (u16)col_1->b * 2) / 3;
        col_3->b = (u8)wrd;
        col_3->a = 0xff;
    }
    else
    {
        // Three-color block: derive the other color.
        // 00 = color_0,  01 = color_1,  10 = color_2,
        // 11 = transparent.
        // These two bit codes correspond to the 2-bit fields
        // stored in the 64-bit block.
        // explicit for each component, unlike some refrasts...
        // //TRACE("block has alpha\n");
        wrd = ((u16)col_0->r + (u16)col_1->r) / 2;
        col_2->r = (u8)wrd;
        wrd = ((u16)col_0->g + (u16)col_1->g) / 2;
        col_2->g = (u8)wrd;
        wrd = ((u16)col_0->b + (u16)col_1->b) / 2;
        col_2->b = (u8)wrd;
        col_2->a = 0xff;
        col_3->r = 0x00; // random color to indicate alpha
        col_3->g = 0xff;
        col_3->b = 0xff;
        col_3->a = 0x00;
    }
} //  Get color block colors (...)

inline void DecodeColorBlock(
u32* pImPos, DXTColBlock* pColorBlock, int width, u32* col_0, u32* col_1, u32* col_2, u32* col_3)
{
    // width is width of image in pixels
    u32 bits;
    int r, n;
    // bit masks = 00000011, 00001100, 00110000, 11000000
    const u32 masks[] = {3, 12, 3 << 4, 3 << 6};
    const int shift[] = {0, 2, 4, 6};
    // r steps through lines in y
    for (r = 0; r < 4; r++, pImPos += width - 4) // no width*4 as u32 ptr inc will *4
    {
        // width * 4 bytes per pixel per line
        // each j dxtc row is 4 lines of pixels
        // pImPos = (DWORD*)((DWORD)pBase + i*16 + (r+j*4) * m_nWidth * 4 );
        // n steps through pixels
        for (n = 0; n < 4; n++)
        {
            bits = pColorBlock->row[r] & masks[n];
            bits >>= shift[n];
            switch (bits)
            {
            case 0:
                *pImPos = *col_0;
                pImPos++; // increment to next u32
                break;
            case 1:
                *pImPos = *col_1;
                pImPos++;
                break;
            case 2:
                *pImPos = *col_2;
                pImPos++;
                break;
            case 3:
                *pImPos = *col_3;
                pImPos++;
                break;
            default:
                // TRACE("Your logic is jacked! bits == 0x%x\n", bits );
                pImPos++;
                break;
            }
        }
    }
}

inline void DecodeAlphaExplicit(u32* pImPos, DXTAlphaBlockExplicit* pAlphaBlock, int width, u32 alphazero)
{
    // alphazero is a bit mask that when & with the image color
    //  will zero the alpha bits, so if the image DWORDs  are
    //  ARGB then alphazero will be 0x00ffffff or if
    //  RGBA then alphazero will be 0xffffff00
    //  alphazero constructed automaticaly from field order of Color8888 structure
    // decodes to 32 bit format only
    u16 wrd;
    Color8888 col;
    col.r = col.g = col.b = 0;
    // TRACE("\n");
    for (int row = 0; row < 4; row++, pImPos += width - 4)
    {
        // pImPow += pImPos += width-4 moves to next row down
        wrd = pAlphaBlock->row[row];
        // //TRACE("0x%.8x\t\t", wrd);
        for (int pix = 0; pix < 4; pix++)
        {
            // zero the alpha bits of image pixel
            *pImPos &= alphazero;
            col.a = wrd & 0x000f; // get only low 4 bits
            // col.a <<= 4; // shift to full byte precision
            // NOTE:  with just a << 4 you'll never have alpha
            // of 0xff,  0xf0 is max so pure shift doesn't quite
            // cover full alpha range.
            // It's much cheaper than divide & scale though.
            // To correct for this, and get 0xff for max alpha,
            //  or the low bits back in after left shifting
            col.a = col.a | (col.a << 4); // This allows max 4 bit alpha to be 0xff alpha
            //  in final image, and is crude approach to full
            //  range scale
            *pImPos |= *(u32*)&col; // or the bits into the prev. nulled alpha
            wrd >>= 4; // move next bits to lowest 4
            pImPos++; // move to next pixel in the row
        }
    }
}

u8 gBits[4][4];
u16 gAlphas[8];
Color8888 gACol[4][4];

inline void DecodeAlpha3BitLinear(u32* pImPos, DXTAlphaBlock3BitLinear* pAlphaBlock, int width, u32 alphazero)
{
    gAlphas[0] = pAlphaBlock->alpha0;
    gAlphas[1] = pAlphaBlock->alpha1;
    // 8-alpha or 6-alpha block?
    if (gAlphas[0] > gAlphas[1])
    {
        // 8-alpha block:  derive the other 6 alphas.
        // 000 = alpha_0, 001 = alpha_1, others are interpolated
        gAlphas[2] = (6 * gAlphas[0] + gAlphas[1]) / 7; // bit code 010
        gAlphas[3] = (5 * gAlphas[0] + 2 * gAlphas[1]) / 7; // Bit code 011
        gAlphas[4] = (4 * gAlphas[0] + 3 * gAlphas[1]) / 7; // Bit code 100
        gAlphas[5] = (3 * gAlphas[0] + 4 * gAlphas[1]) / 7; // Bit code 101
        gAlphas[6] = (2 * gAlphas[0] + 5 * gAlphas[1]) / 7; // Bit code 110
        gAlphas[7] = (gAlphas[0] + 6 * gAlphas[1]) / 7; // Bit code 111
    }
    else
    {
        // 6-alpha block:  derive the other alphas.
        // 000 = alpha_0, 001 = alpha_1, others are interpolated
        gAlphas[2] = (4 * gAlphas[0] + gAlphas[1]) / 5; // Bit code 010
        gAlphas[3] = (3 * gAlphas[0] + 2 * gAlphas[1]) / 5; // Bit code 011
        gAlphas[4] = (2 * gAlphas[0] + 3 * gAlphas[1]) / 5; // Bit code 100
        gAlphas[5] = (gAlphas[0] + 4 * gAlphas[1]) / 5; // Bit code 101
        gAlphas[6] = 0; // Bit code 110
        gAlphas[7] = 255; // Bit code 111
    }
    // Decode 3-bit fields into array of 16 BYTES with same value
    // first two rows of 4 pixels each:
    // pRows = (Alpha3BitRows*) & ( pAlphaBlock->stuff[0] );
    const u32 mask = 0x00000007; // bits = 00 00 01 11
    u32 bits = *(u32*)&pAlphaBlock->stuff[0];
    gBits[0][0] = (u8)(bits & mask);
    bits >>= 3;
    gBits[0][1] = (u8)(bits & mask);
    bits >>= 3;
    gBits[0][2] = (u8)(bits & mask);
    bits >>= 3;
    gBits[0][3] = (u8)(bits & mask);
    bits >>= 3;
    gBits[1][0] = (u8)(bits & mask);
    bits >>= 3;
    gBits[1][1] = (u8)(bits & mask);
    bits >>= 3;
    gBits[1][2] = (u8)(bits & mask);
    bits >>= 3;
    gBits[1][3] = (u8)(bits & mask);
    // now for last two rows:
    bits = *(u32*)&pAlphaBlock->stuff[3]; // last 3 bytes
    gBits[2][0] = (u8)(bits & mask);
    bits >>= 3;
    gBits[2][1] = (u8)(bits & mask);
    bits >>= 3;
    gBits[2][2] = (u8)(bits & mask);
    bits >>= 3;
    gBits[2][3] = (u8)(bits & mask);
    bits >>= 3;
    gBits[3][0] = (u8)(bits & mask);
    bits >>= 3;
    gBits[3][1] = (u8)(bits & mask);
    bits >>= 3;
    gBits[3][2] = (u8)(bits & mask);
    bits >>= 3;
    gBits[3][3] = (u8)(bits & mask);
    // decode the codes into alpha values
    for (int row = 0; row < 4; row++)
    {
        for (int pix = 0; pix < 4; pix++)
        {
            gACol[row][pix].a = (u8)gAlphas[gBits[row][pix]];
            VERIFY(gACol[row][pix].r == 0);
            VERIFY(gACol[row][pix].g == 0);
            VERIFY(gACol[row][pix].b == 0);
        }
    }
    // Write out alpha values to the image bits
    for (int row = 0; row < 4; row++, pImPos += width - 4)
    {
        // pImPow += pImPos += width-4 moves to next row down
        for (int pix = 0; pix < 4; pix++)
        {
            // zero the alpha bits of image pixel
            *pImPos &= alphazero;
            // or the bits into the prev. nulled alpha
            *pImPos |= *((u32*)&(gACol[row][pix]));
            pImPos++;
        }
    }
}

void Image_DXTC::DecompressDXT1()
{
    // This was hacked up pretty quick & slopily
    // decompresses to 32 bit format 0xARGB
    int xblocks = m_DDSD.dwWidth / 4;
    int yblocks = m_DDSD.dwHeight / 4;
    u32* pBase = (u32*)m_pDecompBytes;
    u32* pImPos = (u32*)pBase; // pos in decompressed data
    u16* pPos = (u16*)m_pCompBytes; // pos in compressed data
    DXTColBlock* pBlock;
    Color8888 col_0, col_1, col_2, col_3;
    u16 wrd;
    // TRACE("blocks: x: %d    y: %d\n", xblocks, yblocks );
    for (int j = 0; j < yblocks; j++)
    {
        // 8 bytes per block
        pBlock = (DXTColBlock*)((std::uintptr_t)m_pCompBytes + j * xblocks * 8);
        for (int i = 0; i < xblocks; i++, pBlock++)
        {
            // inline func:
            GetColorBlockColors(pBlock, &col_0, &col_1, &col_2, &col_3, wrd);
            // now decode the color block into the bitmap bits
            // inline func:
            pImPos = (u32*)((std::uintptr_t)pBase + i * 16 + (j * 4) * m_nWidth * 4);
            DecodeColorBlock(pImPos, pBlock, m_nWidth, (u32*)&col_0, (u32*)&col_1, (u32*)&col_2, (u32*)&col_3);
            if (false) // Set to RGB test pattern
            {
                pImPos = (u32*)((std::uintptr_t)pBase + i * 4 + j * m_nWidth * 4);
                *pImPos = ((i * 4) << 16) | ((j * 4) << 8) | ((63 - i) * 4);
                // checkerboard of only col_0 and col_1 basis colors:
                pImPos = (u32*)((std::uintptr_t)pBase + i * 8 + j * m_nWidth * 8);
                *pImPos = *((u32*)&col_0);
                pImPos += 1 + m_nWidth;
                *pImPos = *((u32*)&col_1);
            }
        }
    }
}

void Image_DXTC::DecompressDXT2()
{
    // Can do color & alpha same as dxt3, but color is pre-multiplied
    //   so the result will be wrong unless corrected.
    // DecompressDXT3();
    VERIFY(false);
}

void Image_DXTC::DecompressDXT3()
{
    int xblocks = m_DDSD.dwWidth / 4;
    int yblocks = m_DDSD.dwHeight / 4;
    u32* pBase = (u32*)m_pDecompBytes;
    u32* pImPos = (u32*)pBase; // pos in decompressed data
    u16* pPos = (u16*)m_pCompBytes; // pos in compressed data
    DXTColBlock* pBlock;
    DXTAlphaBlockExplicit* pAlphaBlock;
    Color8888 col_0, col_1, col_2, col_3;
    u16 wrd;
    // fill alphazero with appropriate value to zero out alpha when
    //  alphazero is ANDed with the image color 32 bit DWORD:
    col_0.a = 0;
    col_0.r = col_0.g = col_0.b = 0xff;
    u32 alphazero = *((u32*)&col_0);
    //	//TRACE("blocks: x: %d    y: %d\n", xblocks, yblocks );
    for (int j = 0; j < yblocks; j++)
    {
        // 8 bytes per block
        // 1 block for alpha, 1 block for color
        pBlock = (DXTColBlock*)((std::uintptr_t)m_pCompBytes + j * xblocks * 16);
        for (int i = 0; i < xblocks; i++, pBlock++)
        {
            // inline
            // Get alpha block
            pAlphaBlock = (DXTAlphaBlockExplicit*)pBlock;
            // inline func:
            // Get color block & colors
            pBlock++;
            GetColorBlockColors(pBlock, &col_0, &col_1, &col_2, &col_3, wrd);
            // Decode the color block into the bitmap bits inline func:
            pImPos = (u32*)((std::uintptr_t)pBase + i * 16 + (j * 4) * m_nWidth * 4);
            DecodeColorBlock(pImPos, pBlock, m_nWidth, (u32*)&col_0, (u32*)&col_1, (u32*)&col_2, (u32*)&col_3);
            // Overwrite the previous alpha bits with the alpha block
            //  info
            // inline func:
            DecodeAlphaExplicit(pImPos, pAlphaBlock, m_nWidth, alphazero);
        }
    }
}

void Image_DXTC::DecompressDXT4()
{
    // Can do color & alpha same as dxt5, but color is pre-multiplied
    //   so the result will be wrong unless corrected.
    // DecompressDXT5();
    VERIFY(false);
}

void Image_DXTC::DecompressDXT5()
{
    int xblocks = m_DDSD.dwWidth / 4;
    int yblocks = m_DDSD.dwHeight / 4;
    u32* pBase = (u32*)m_pDecompBytes;
    u32* pImPos = (u32*)pBase; // pos in decompressed data
    u16* pPos = (u16*)m_pCompBytes; // pos in compressed data
    Color8888 col_0, col_1, col_2, col_3;
    // fill alphazero with appropriate value to zero out alpha when
    //  alphazero is ANDed with the image color 32 bit u32:
    col_0.a = 0;
    col_0.r = col_0.g = col_0.b = 0xff;
    u32 alphazero = *(u32*)&col_0;
    u16 wrd;
    //	//TRACE("blocks: x: %d    y: %d\n", xblocks, yblocks );
    for (int j = 0; j < yblocks; j++)
    {
        // 8 bytes per block
        // 1 block for alpha, 1 block for color
        DXTColBlock* pBlock = (DXTColBlock*)((std::uintptr_t)m_pCompBytes + j * xblocks * 16);
        for (int i = 0; i < xblocks; i++, pBlock++)
        {
            // inline
            // Get alpha block
            DXTAlphaBlock3BitLinear* pAlphaBlock = (DXTAlphaBlock3BitLinear*)pBlock;
            // inline func:
            // Get color block & colors
            pBlock++;
            // //TRACE("pBlock:   0x%.8x\n", pBlock );
            GetColorBlockColors(pBlock, &col_0, &col_1, &col_2, &col_3, wrd);
            // Decode the color block into the bitmap bits inline func:
            pImPos = (u32*)((std::uintptr_t)pBase + i * 16 + (j * 4) * m_nWidth * 4);
            DecodeColorBlock(pImPos, pBlock, m_nWidth, (u32*)&col_0, (u32*)&col_1, (u32*)&col_2, (u32*)&col_3);
            // Overwrite the previous alpha bits with the alpha block info
            DecodeAlpha3BitLinear(pImPos, pAlphaBlock, m_nWidth, alphazero);
        }
    }
} // dxt5

//-----------------------------------------------------------------------------
// Name: PixelFormatToString()
// Desc: Creates a string describing a pixel format.
//	adapted from microsoft mssdk D3DIM Compress example
//  PixelFormatToString()
//-----------------------------------------------------------------------------
void Image_DXTC::DecodePixelFormat(char* strPixelFormat, DDS_PIXELFORMAT* pddpf)
{
    switch (pddpf->dwFourCC)
    {
    case 0:
        // This dds texture isn't compressed so write out ARGB format
        sprintf(strPixelFormat, "ARGB-%d%d%d%d%s", GetNumberOfBits(pddpf->dwRGBAlphaBitMask),
        GetNumberOfBits(pddpf->dwRBitMask), GetNumberOfBits(pddpf->dwGBitMask), GetNumberOfBits(pddpf->dwBBitMask),
        pddpf->dwFlags & DDPF_ALPHAPREMULT ? "-premul" : "");
        m_CompFormat = PF_ARGB;
        break;
    case MAKEFOURCC('D', 'X', 'T', '1'):
        strcpy(strPixelFormat, "DXT1");
        m_CompFormat = PF_DXT1;
        break;
    case MAKEFOURCC('D', 'X', 'T', '2'):
        strcpy(strPixelFormat, "DXT2");
        m_CompFormat = PF_DXT2;
        break;
    case MAKEFOURCC('D', 'X', 'T', '3'):
        strcpy(strPixelFormat, "DXT3");
        m_CompFormat = PF_DXT3;
        break;
    case MAKEFOURCC('D', 'X', 'T', '4'):
        strcpy(strPixelFormat, "DXT4");
        m_CompFormat = PF_DXT4;
        break;
    case MAKEFOURCC('D', 'X', 'T', '5'):
        strcpy(strPixelFormat, "DXT5");
        m_CompFormat = PF_DXT5;
        break;
    default:
        strcpy(strPixelFormat, "Format Unknown");
        m_CompFormat = PF_UNKNOWN;
        break;
    }
}

inline void GetColorBlockColors_m2(
DXTColBlock* pBlock, Color8888* col_0, Color8888* col_1, Color8888* col_2, Color8888* col_3, u16& wrd)
{
    // method 2
    // freak variable bit structure method
    // normal math
    Color565* pCol;
    pCol = (Color565*)&pBlock->col0;
    col_0->a = 0xff;
    col_0->r = pCol->nRed;
    col_0->r <<= 3; // shift to full precision
    col_0->g = pCol->nGreen;
    col_0->g <<= 2;
    col_0->b = pCol->nBlue;
    col_0->b <<= 3;
    pCol = (Color565*)&pBlock->col1;
    col_1->a = 0xff;
    col_1->r = pCol->nRed;
    col_1->r <<= 3; // shift to full precision
    col_1->g = pCol->nGreen;
    col_1->g <<= 2;
    col_1->b = pCol->nBlue;
    col_1->b <<= 3;
    if (pBlock->col0 > pBlock->col1)
    {
        // Four-color block: derive the other two colors.
        // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
        // These two bit codes correspond to the 2-bit fields
        // stored in the 64-bit block.
        wrd = ((u16)col_0->r * 2 + (u16)col_1->r) / 3;
        // no +1 for rounding
        // as bits have been shifted to 888
        col_2->r = (u8)wrd;
        wrd = ((u16)col_0->g * 2 + (u16)col_1->g) / 3;
        col_2->g = (u8)wrd;
        wrd = ((u16)col_0->b * 2 + (u16)col_1->b) / 3;
        col_2->b = (u8)wrd;
        col_2->a = 0xff;
        wrd = ((u16)col_0->r + (u16)col_1->r * 2) / 3;
        col_3->r = (u8)wrd;
        wrd = ((u16)col_0->g + (u16)col_1->g * 2) / 3;
        col_3->g = (u8)wrd;
        wrd = ((u16)col_0->b + (u16)col_1->b * 2) / 3;
        col_3->b = (u8)wrd;
        col_3->a = 0xff;
    }
    else
    {
        // Three-color block: derive the other color.
        // 00 = color_0,  01 = color_1,  10 = color_2,
        // 11 = transparent.
        // These two bit codes correspond to the 2-bit fields
        // stored in the 64-bit block.
        // explicit for each component, unlike some refrasts...
        // //TRACE("block has alpha\n");
        wrd = ((u16)col_0->r + (u16)col_1->r) / 2;
        col_2->r = (u8)wrd;
        wrd = ((u16)col_0->g + (u16)col_1->g) / 2;
        col_2->g = (u8)wrd;
        wrd = ((u16)col_0->b + (u16)col_1->b) / 2;
        col_2->b = (u8)wrd;
        col_2->a = 0xff;
        col_3->r = 0x00; // random color to indicate alpha
        col_3->g = 0xff;
        col_3->b = 0xff;
        col_3->a = 0x00;
    }
}

inline void GetColorBlockColors_m3(
DXTColBlock* pBlock, Color8888* col_0, Color8888* col_1, Color8888* col_2, Color8888* col_3, u16& wrd)
{
    // method 3
    // super-freak variable bit structure with
    //  Cool Math Trick (tm)
    // Do 2/3 1/3 math BEFORE bit shift on the whole DWORD
    // as the fields will NEVER carry into the next
    //  or overflow!! =)
    Color565* pCol;
    pCol = (Color565*)&pBlock->col0;
    col_0->a = 0x00; // must set to 0 to avoid overflow in DWORD add
    col_0->r = pCol->nRed;
    col_0->g = pCol->nGreen;
    col_0->b = pCol->nBlue;
    pCol = (Color565*)&pBlock->col1;
    col_1->a = 0x00;
    col_1->r = pCol->nRed;
    col_1->g = pCol->nGreen;
    col_1->b = pCol->nBlue;
    if (pBlock->col0 > pBlock->col1)
    {
        *(u32*)col_2 = *(u32*)col_0 * 2 + *(u32*)col_1;
        *(u32*)col_3 = *(u32*)col_0 + *(u32*)col_1 * 2;
        // now shift to appropriate precision & divide by 3.
        col_2->r = ((u16)col_2->r << 3) / (u16)3;
        col_2->g = ((u16)col_2->g << 2) / (u16)3;
        col_2->b = ((u16)col_2->b << 3) / (u16)3;
        col_3->r = ((u16)col_3->r << 3) / (u16)3;
        col_3->g = ((u16)col_3->g << 2) / (u16)3;
        col_3->b = ((u16)col_3->b << 3) / (u16)3;
        col_0->a = 0xff; // now set appropriate alpha
        col_1->a = 0xff;
        col_2->a = 0xff;
        col_3->a = 0xff;
    }
    else
    {
        *(u32*)col_2 = *(u32*)col_0 + *(u32*)col_1;
        // now shift to appropriate precision & divide by 2.
        // << 3 ) / 2 == << 2
        // << 2 ) / 2 == << 1
        col_2->r = ((u16)col_2->r << 2);
        col_2->g = ((u16)col_2->g << 1);
        col_2->b = ((u16)col_2->b << 2);
        col_2->a = 0xff;
        col_3->a = 0x00;
        col_3->r = 0x00; // random color to indicate alpha
        col_3->g = 0xff;
        col_3->b = 0xff;
    }
    // now shift orig color components
    col_0->r <<= 3;
    col_0->g <<= 2;
    col_0->b <<= 3;
    col_1->r <<= 3;
    col_1->g <<= 2;
    col_1->b <<= 3;
}

inline void GetColorBlockColors_m4(
DXTColBlock* pBlock, Color8888* col_0, Color8888* col_1, Color8888* col_2, Color8888* col_3, u16& wrd)
{
    // m1 color extraction from 5-6-5
    // m3 color math on DWORD before bit shift to full precision
    wrd = pBlock->col0;
    col_0->a = 0x00; // must set to 0 to avoid possible overflow & carry to next field in DWORD add
    // extract r,g,b bits
    col_0->b = (unsigned char)wrd & 0x1f; // 0x1f = 0001 1111  to mask out upper 3 bits
    wrd >>= 5;
    col_0->g = (unsigned char)wrd & 0x3f; // 0x3f = 0011 1111  to mask out upper 2 bits
    wrd >>= 6;
    col_0->r = (unsigned char)wrd & 0x1f;
    // same for col # 2:
    wrd = pBlock->col1;
    col_1->a = 0x00; // must set to 0 to avoid possible overflow in DWORD add
    // extract r,g,b bits
    col_1->b = (unsigned char)wrd & 0x1f;
    wrd >>= 5;
    col_1->g = (unsigned char)wrd & 0x3f;
    wrd >>= 6;
    col_1->r = (unsigned char)wrd & 0x1f;
    if (pBlock->col0 > pBlock->col1)
    {
        *(u32*)col_2 = (*(u32*)col_0 * 2 + *(u32*)col_1);
        *(u32*)col_3 = (*(u32*)col_0 + *(u32*)col_1 * 2);
        // shift to appropriate precision & divide by 3.
        col_2->r = ((u16)col_2->r << 3) / (u16)3;
        col_2->g = ((u16)col_2->g << 2) / (u16)3;
        col_2->b = ((u16)col_2->b << 3) / (u16)3;
        col_3->r = ((u16)col_3->r << 3) / (u16)3;
        col_3->g = ((u16)col_3->g << 2) / (u16)3;
        col_3->b = ((u16)col_3->b << 3) / (u16)3;
        col_0->a = 0xff; // set appropriate alpha
        col_1->a = 0xff;
        col_2->a = 0xff;
        col_3->a = 0xff;
    }
    else
    {
        *(u32*)col_2 = *(u32*)col_0 + *(u32*)col_1;
        // shift to appropriate precision & divide by 2.
        // << 3 ) / 2 == << 2
        // << 2 ) / 2 == << 1
        col_2->r = ((u16)col_2->r << 2);
        col_2->g = ((u16)col_2->g << 1);
        col_2->b = ((u16)col_2->b << 2);
        col_2->a = 0xff;
        col_3->a = 0x00;
        col_3->r = 0x00; // random color to indicate alpha
        col_3->g = 0xff;
        col_3->b = 0xff;
    }
    // shift orig color components to full precision
    col_0->r <<= 3;
    col_0->g <<= 2;
    col_0->b <<= 3;
    col_1->r <<= 3;
    col_1->g <<= 2;
    col_1->b <<= 3;
}

inline void GetColorBlockColors_m1(
DXTColBlock* pBlock, Color8888* col_0, Color8888* col_1, Color8888* col_2, Color8888* col_3, u16& wrd)
{
    // Method 1:
    // Shifty method
    wrd = pBlock->col0;
    col_0->a = 0xff;
    // extract r,g,b bits
    col_0->b = (unsigned char)wrd;
    col_0->b <<= 3; // shift to full precision
    wrd >>= 5;
    col_0->g = (unsigned char)wrd;
    col_0->g <<= 2; // shift to full precision
    wrd >>= 6;
    col_0->r = (unsigned char)wrd;
    col_0->r <<= 3; // shift to full precision
    // same for col # 2:
    wrd = pBlock->col1;
    col_1->a = 0xff;
    // extract r,g,b bits
    col_1->b = (unsigned char)wrd;
    col_1->b <<= 3; // shift to full precision
    wrd >>= 5;
    col_1->g = (unsigned char)wrd;
    col_1->g <<= 2; // shift to full precision
    wrd >>= 6;
    col_1->r = (unsigned char)wrd;
    col_1->r <<= 3; // shift to full precision
    // use this for all but the super-freak math method
    if (pBlock->col0 > pBlock->col1)
    {
        // Four-color block: derive the other two colors.
        // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
        // These two bit codes correspond to the 2-bit fields
        // stored in the 64-bit block.
        wrd = ((u16)col_0->r * 2 + (u16)col_1->r) / 3;
        // no +1 for rounding
        // as bits have been shifted to 888
        col_2->r = (u8)wrd;
        wrd = ((u16)col_0->g * 2 + (u16)col_1->g) / 3;
        col_2->g = (u8)wrd;
        wrd = ((u16)col_0->b * 2 + (u16)col_1->b) / 3;
        col_2->b = (u8)wrd;
        col_2->a = 0xff;
        wrd = ((u16)col_0->r + (u16)col_1->r * 2) / 3;
        col_3->r = (u8)wrd;
        wrd = ((u16)col_0->g + (u16)col_1->g * 2) / 3;
        col_3->g = (u8)wrd;
        wrd = ((u16)col_0->b + (u16)col_1->b * 2) / 3;
        col_3->b = (u8)wrd;
        col_3->a = 0xff;
    }
    else
    {
        // Three-color block: derive the other color.
        // 00 = color_0,  01 = color_1,  10 = color_2,
        // 11 = transparent.
        // These two bit codes correspond to the 2-bit fields
        // stored in the 64-bit block.
        // explicit for each component, unlike some refrasts...
        // //TRACE("block has alpha\n");
        wrd = ((u16)col_0->r + (u16)col_1->r) / 2;
        col_2->r = (u8)wrd;
        wrd = ((u16)col_0->g + (u16)col_1->g) / 2;
        col_2->g = (u8)wrd;
        wrd = ((u16)col_0->b + (u16)col_1->b) / 2;
        col_2->b = (u8)wrd;
        col_2->a = 0xff;
        col_3->r = 0x00; // random color to indicate alpha
        col_3->g = 0xff;
        col_3->b = 0xff;
        col_3->a = 0x00;
    }
} //  Get color block colors (...)

//-----------------------------------------------------------------------------
// Name: GetNumberOfBits()
// Desc: Returns the number of bits set in a DWORD mask
//	from microsoft mssdk d3dim sample "Compress"
//-----------------------------------------------------------------------------
u16 GetNumberOfBits(u32 dwMask)
{
    u16 wBits = 0;
    while (dwMask)
    {
        dwMask &= dwMask - 1;
        wBits++;
    }
    return wBits;
}
