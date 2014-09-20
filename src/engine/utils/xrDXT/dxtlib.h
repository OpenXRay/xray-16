/****************************************************************************************
	
    Copyright (C) NVIDIA Corporation 2003

    TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
    *AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
    OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY
    AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS
    BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
    WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
    BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
    ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
    BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

*****************************************************************************************/
#pragma once

#include "nvdxt_options.h"

typedef HRESULT (__cdecl *MIPcallback)(void * data, int miplevel, DWORD size, int width, int height, void * user_data);


// call back
// pointer to data
// mip level
// size of chunk

 


inline bool IsPower2(unsigned int x)
{              
    if ( x < 1 )
        return false;

    if (x == 1)
        return true;

    if ( x & (x-1) )        
        return false;

    return true;
}




inline bool IsMultiple4(unsigned int x)
{              
    if ( x == 0 )
        return false;

    if ((x % 4) == 0)
        return true;
    else
        return false;
}


/*
   Compresses an image with a user supplied callback with the data for each MIP level created
   Only supports input of RGB 24 or ARGB 32 bpp
*/

#ifdef NVDXTC
extern "C" {
#endif


// define color order
HRESULT nvDXTcompressRGBA(unsigned char * raw_data, // pointer to data (24 or 32 bit)
                unsigned long w, // width in texels
                unsigned long h, // height in texels
                DWORD byte_pitch,
                CompressionOptions * options,
                DWORD planes, // 3 or 4
                MIPcallback callback = NULL,  // callback for generated levels
                RECT * rect = NULL);   // subrect to operate on, NULL is whole image



HRESULT nvDXTcompressBGRA(unsigned char * image_data, 
                      unsigned long image_width,
                      unsigned long image_height, 
                      DWORD byte_pitch,
                      CompressionOptions * options,
                      DWORD planes,
                      MIPcallback callback = 0,
                      RECT * rect = 0);


HRESULT nvDXTcompressVolumeBGRA(unsigned char * raw_data, // pointer to data (24 or 32 bit)
                unsigned long w, // width in texels
                unsigned long h, // height in texels
                unsigned long depth, // depth of volume texture
                DWORD byte_pitch,
                CompressionOptions * options,
                DWORD planes, // 3 or 4
                MIPcallback callback = NULL,  // callback for generated levels
                RECT * rect = NULL);   // subrect to operate on, NULL is whole image


HRESULT nvDXTcompressVolumeRGBA(unsigned char * raw_data, // pointer to data (24 or 32 bit)
                unsigned long w, // width in texels
                unsigned long h, // height in texels
                unsigned long depth, // depth of volume texture
                DWORD byte_pitch,
                CompressionOptions * options,
                DWORD planes, // 3 or 4
                MIPcallback callback = NULL,  // callback for generated levels
                RECT * rect = NULL);   // subrect to operate on, NULL is whole image


// floating point input
HRESULT nvDXTcompress32F(fpImage & srcImage,
                CompressionOptions * options,
                MIPcallback callback = NULL,  // callback for generated levels
                RECT * rect = NULL);   // subrect to operate on, NULL is whole image


HRESULT __cdecl nvDXTcompress(RGBAImage & image,
                      CompressionOptions * options,
                      MIPcallback callback,
                      RECT * rect);
#ifdef  NVDXTC
}
#endif

// if callback is == 0 (or not specified), then WriteDTXnFile is called with all file info
//
// You must write the routines (or provide stubs)
// void WriteDTXnFile(count, buffer, userData);
// void ReadDTXnFile(count, buffer, userData);
// 
//
#ifdef  NVDXTDLL

typedef void (*DXTDataTransfer)(DWORD count, void *buffer, void *);

#ifdef  NVDXTC
extern "C" {
#endif

void SetReadDTXnFile(DXTDataTransfer UserReadDTXnFile);
void SetWriteDTXnFile(DXTDataTransfer UserWriteDTXnFile);


#ifdef  NVDXTC
}
#endif

#else

void __cdecl WriteDTXnFile(DWORD count, void * buffer, void * userData);
void __cdecl ReadDTXnFile(DWORD count, void * buffer, void * userData);


/*
#ifndef EXCLUDE_LIBS

#if _DEBUG

 #if _MSC_VER >=1300
  #ifdef _MT
   #ifdef _DLL
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlibMTDLL_Sd.lib") 
     #pragma comment(lib, "nvDXTlibMTDLL_Sd.lib")
    #else
     #pragma message("Note: including lib: nvDXTlibMTDLLd.lib") 
     #pragma comment(lib, "nvDXTlibMTDLLd.lib")
    #endif
   #else // DLL
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlibMT_Sd.lib") 
     #pragma comment(lib, "nvDXTlibMT_Sd.lib")
    #else
     #pragma message("Note: including lib: nvDXTlibMTd.lib") 
     #pragma comment(lib, "nvDXTlibMTd.lib")
    #endif
   #endif //_DLL
  #else // MT
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlib_Sd.lib") 
     #pragma comment(lib, "nvDXTlib_Sd.lib")
    #else
     #pragma message("Note: including lib: nvDXTlibd.lib") 
     #pragma comment(lib, "nvDXTlibd.lib")
    #endif
  #endif // _MT
 #else // _MSC_VER

  #ifdef _MT
   #ifdef _DLL                         
    #pragma message("Note: including lib: nvDXTlibMTDLL6.lib") 
    #pragma comment(lib, "nvDXTlibMTDLL6.lib")
   #else // _DLL
    #pragma message("Note: including lib: nvDXTlibMT6.lib") 
    #pragma comment(lib, "nvDXTlibMT6.lib")
   #endif //_DLL
  #else // _MT
   #pragma message("Note: including lib: nvDXTlib6.lib") 
   #pragma comment(lib, "nvDXTlib6.lib")
  #endif // _MT
 
 #endif // _MSC_VER

#else // _DEBUG


 #if _MSC_VER >=1300
  #ifdef _MT
   #ifdef _DLL
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlibMTDLL_S.lib") 
     #pragma comment(lib, "nvDXTlibMTDLL_S.lib")
    #else
     #pragma message("Note: including lib: nvDXTlibMTDLL.lib") 
     #pragma comment(lib, "nvDXTlibMTDLL.lib")
    #endif
   #else // DLL
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlibMT_S.lib") 
     #pragma comment(lib, "nvDXTlibMT_S.lib")
    #else
     #pragma message("Note: including lib: nvDXTlibMT.lib") 
     #pragma comment(lib, "nvDXTlibMT.lib")
    #endif
   #endif //_DLL
  #else // MT
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlib_S.lib") 
     #pragma comment(lib, "nvDXTlib_S.lib")
    #else
     #pragma message("Note: including lib: nvDXTlib.lib") 
     #pragma comment(lib, "nvDXTlib.lib")
    #endif
  #endif // _MT
 #else // _MSC_VER

  #ifdef _MT
   #ifdef _DLL                         
    #pragma message("Note: including lib: nvDXTlibMTDLL6.lib") 
    #pragma comment(lib, "nvDXTlibMTDLL6.lib")
   #else // _DLL
    #pragma message("Note: including lib: nvDXTlibMT6.lib") 
    #pragma comment(lib, "nvDXTlibMT6.lib")
   #endif //_DLL
  #else // _MT
   #pragma message("Note: including lib: nvDXTlib6.lib") 
   #pragma comment(lib, "nvDXTlib6.lib")
  #endif // _MT
 
 #endif // _MSC_VER
#endif // _DEBUG





#endif
*/

#endif // NVDXTC




#define DXTERR_INPUT_POINTER_ZERO -1
#define DXTERR_DEPTH_IS_NOT_3_OR_4 -2
#define DXTERR_NON_POWER_2 -3
#define DXTERR_INCORRECT_NUMBER_OF_PLANES -4
#define DXTERR_NON_MUL4 -5


/* example

LPDIRECT3DTEXTURE8 pCurrentTexture = 0; 

HRESULT LoadAllMipSurfaces(void * data, int iLevel)
{
    HRESULT hr;
    LPDIRECT3DSURFACE8 psurf;
    D3DSURFACE_DESC sd;
    D3DLOCKED_RECT lr;
       
    hr = pCurrentTexture->GetSurfaceLevel(iLevel, &psurf);
    
    if (FAILED(hr))
        return hr;
    psurf->GetDesc(&sd);
    
    
    hr = pCurrentTexture->LockRect(iLevel, &lr, NULL, 0);
    if (FAILED(hr))
        return hr;
    
    memcpy(lr.pBits, data, sd.Size);
    
    hr = pCurrentTexture->UnlockRect(iLevel);
    
    ReleasePpo(&psurf);
    
    return 0;
}
       

    hr = D3DXCreateTexture(m_pd3dDevice, Width, Height, nMips,  0,   D3DFMT_DXT3,  D3DPOOL_MANAGED, &pCurrentTexture);
    nvDXTcompress(raw_data, Width, Height, DXT3, true, 4, LoadAllMipSurfaces, NULL);

*/


	/*
    src_format
    dDXT1 
	dDXT1a  // DXT1 with one bit alpha
	dDXT3    // explicit alpha
	dDXT5    // interpolated alpha

	d4444   // a4 r4 g4 b4
	d1555   // a1 r5 g5 b5
	d565    // a0 r5 g6 b5
	d8888   // a8 r8 g8 b8
	d888    // a0 r8 g8 b8
	d555    // a0 r5 g5 b5
    d8      // paletted
    dV8U8   // DuDv
    dCxV8U8   // normal map
    dA8       // A8

      */

#ifdef NVDXTC
extern "C" {
#endif

/*
  
  SpecifiedMipMaps, number of MIP maps to load. 0 is all


*/

unsigned char * nvDXTdecompress(int & w, int & h, int & depth, int & total_width, int & rowBytes, int & src_format,
                                int SpecifiedMipMaps = 0);


#ifdef NVDXTC
}
#endif

#ifndef COLOR_FORMAT_ENUM
#define COLOR_FORMAT_ENUM

enum ColorFormat
{
	COLOR_RGB,
	COLOR_ARGB,
	COLOR_BGR,
	COLOR_BGRA,
	COLOR_RGBA,
	COLOR_ABGR,
};

#endif

