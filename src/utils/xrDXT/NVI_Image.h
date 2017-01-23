/*********************************************************************NVMH2****
Path:  C:\Dev\devrel\Nv_sdk_4\CommonSrc\nvImageLib
File:  NVI_Image.h

Copyright (C) 1999, 2000 NVIDIA Corporation
This file is provided without support, instruction, or implied warranty of any
kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
not liable under any circumstances for any damages or loss whatsoever arising
from the use or inability to use this file or items derived from it.

Comments:

******************************************************************************/

#ifndef  __NVIMAGELIB_NVI_IMAGE_H
#define  __NVIMAGELIB_NVI_IMAGE_H

#include <windows.h>
#include <assert.h>
#include "NV_Common.h"
// Debug test switch
// Define to turn on debug _ASSERT()s
// #define		NVIHDEBUG

namespace xray_nvi
{
    enum NVI_PIXEL_FORMAT
    {
        IMAGE_NOT_INITIALIZED,
        NVI_A8,
        //@	NVI_R8_G8_B8_A8, // unsigned integer - A is most sig byte
        NVI_A8_R8_G8_B8, // unsigned integer - A is most sig byte (0xAARRGGBB)        
        NVI_A1_R5_G5_B5,
        NVI_R5_G6_B5,
        NVI_A16, // 16-bit unsigned integer
        //@	NVI_R16_G16_B16,
        NVI_R16_G16_B16_A16,
        NVI_FMT_FORCEDWORD = 0xFFFFFFFF
    };
    
    class NVI_Image
    {
    public:
        u8* m_pArray;
        NVI_PIXEL_FORMAT m_Format;
        // Ints so that underflow does not wrap!
        int m_nSizeX;
        int m_nSizeY;

    public:
        NVI_Image();
        ~NVI_Image();
        virtual HRESULT Initialize(int width, int height, NVI_PIXEL_FORMAT format);
        virtual HRESULT Initialize(int width, int height, NVI_PIXEL_FORMAT format, u8* data);
        virtual HRESULT Free();
        UINT GetBytesPerPixel();
        UINT GetImageNumBytes();
        NVI_PIXEL_FORMAT GetFormat() { return m_Format; }
        UINT GetWidth() { return m_nSizeX; }
        UINT GetHeight() { return m_nSizeY; }
        UINT GetNumPixels();
        BYTE* GetImageDataPointer() { return m_pArray; }
        bool IsDataValid();
        void FlipTopToBottom();
        void AverageRGBToAlpha(); // write each pixels' avg r,g,b to alpha
        void ABGR8_To_ARGB8();

    private:
        void GetPixel_ARGB8(DWORD* outPix, UINT i, UINT j);
        void SetPixel_ARGB8(UINT i, UINT j, DWORD pix);
        void GetPixel_ARGB8(DWORD* outPix, UINT index);
        void SetPixel_ARGB8(UINT index, DWORD pix);

        friend class NVI_PNG_File;
        friend class NVI_GraphicsFile;
        friend class NVI_ImageBordered;
    };

    // Inline functions
    // Should not do any new or delete here

    __forceinline void NVI_Image::GetPixel_ARGB8(DWORD* outPix, UINT i, UINT j)
    {
#ifdef NVIHDEBUG
        // _ASSERT because that evaluates for debug only
        // assert() evaluates for debug and release
        _ASSERT(GetFormat() == NVI_A8_R8_G8_B8);
        _ASSERT(outPix != NULL);
#endif
        *outPix = ((DWORD*)m_pArray)[j * m_nSizeX + i];
    }

    __forceinline void NVI_Image::SetPixel_ARGB8(UINT i, UINT j, DWORD pix)
    {
#ifdef NVIHDEBUG
        _ASSERT(GetFormat() == NVI_A8_R8_G8_B8);
#endif
        ((DWORD*)m_pArray)[j * m_nSizeX + i] = pix;
    }

    __forceinline void NVI_Image::GetPixel_ARGB8(DWORD* outPix, UINT index)
    {
#ifdef NVIHDEBUG
        // _ASSERT because that evaluates for debug only
        // assert() evaluates for debug and release
        _ASSERT(GetFormat() == NVI_A8_R8_G8_B8);
        _ASSERT(outPix != NULL);
#endif
        *outPix = ((DWORD*)m_pArray)[index];
    }

    __forceinline void NVI_Image::SetPixel_ARGB8(UINT index, DWORD pix)
    {
#ifdef NVIHDEBUG
        _ASSERT(GetFormat() == NVI_A8_R8_G8_B8);
#endif
        ((DWORD*)m_pArray)[index] = pix;
    }

    class NVI_ImageBordered : public NVI_Image
    {
    private:
        int m_nBorderXLow; // ** Negative or zero ** 
        // This is the offset from source images' 0,0 
        //   that marks the left border.
        //	 Border width on the left = -m_nBorderXLow;
        //   x = 0 that pixels can be addressed
        int m_nBorderXHigh; // Size of border to 'right' of image.  >= 0
        int m_nBorderYLow; // Same thing for Y borders
        int m_nBorderYHigh;
        NVI_Image** m_hSrcImage; // Image from which this was created
        bool m_bWrap; // Wrap or clamp the border pixels

        void CopyDataFromSource();

    public:
        NVI_ImageBordered();
        ~NVI_ImageBordered();
        HRESULT Initialize(NVI_Image** hSrcImage, const RECT* border, bool wrap);
        HRESULT Free();
        // i,j relative to src image, so i,j = 0 fetches from
        //   (i-m_nBorderXLow, j-m_nBorderYLow ) in the m_pArray
        void GetPixel(DWORD* pDest, int i, int j);
    };

    // Inline functions
    // Should not do any new or delete here

    // i,j relative to src image, so i,j = 0 fetches from
    //   (i-m_nBorderXLow, j-m_nBorderYLow ) in the m_pArray
    __forceinline void NVI_ImageBordered::GetPixel(DWORD* outColor, int i, int j)
    {
        *outColor = ((DWORD*)m_pArray)[(j - m_nBorderYLow) * m_nSizeX + (i - m_nBorderXLow)];
    }
};

#endif // __NVIMAGELIB_NVI_IMAGE_H
