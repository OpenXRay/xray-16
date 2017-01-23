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

#ifndef DXTLIB_API
#define DXTLIB_API
#endif

#include <dds/nvdxt_options.h>
#include <dds/nvErrorCodes.h>

struct MIPMapData
{
    size_t mipLevel; 
    size_t width;
    size_t height;
    int faceNumber; // current face number for this image
    int numFaces; // total number of faces (depth for volume textures, 6 for cube maps)
};

// Write callback function. 
typedef NV_ERROR_CODE (*DXTReadCallback)(void* buffer, size_t count, void* userData);

typedef NV_ERROR_CODE (*DXTWriteCallback)(
	const void* buffer, 
	size_t count, 
	const MIPMapData* mipMapData, // if nz, this is MIP data
    void* userData);

// Compresses an image with a user supplied callback with the data for each MIP level created
typedef enum nvPixelOrder
{
    nvRGBA,
    nvBGRA,
    nvRGB,
    nvBGR,
    nvGREY,  // one plance copied to RGB
};

namespace nvDDS
{
    DXTLIB_API NV_ERROR_CODE nvDXTcompress(const nvImageContainer& imageContainer,
        nvCompressionOptions* options,
        DXTWriteCallback fileWriteRoutine,  // call to .dds write routine
        const RECT* rect = 0);

    DXTLIB_API NV_ERROR_CODE nvDXTcompress(const RGBAImage & srcImage,
        nvCompressionOptions* options,
        DXTWriteCallback fileWriteRoutine,  // call to .dds write routine
        const RECT* rect = 0);

    DXTLIB_API NV_ERROR_CODE nvDXTcompress(const unsigned char * srcImage,
        size_t width,
        size_t height,
        size_t byte_pitch,
        nvPixelOrder pixelOrder,
        nvCompressionOptions* options,
        DXTWriteCallback fileWriteRoutine, // call to .dds write routine
        const RECT* rect = 0);
    
    // image with MIP maps
    DXTLIB_API NV_ERROR_CODE nvDXTcompress(const RGBAMipMappedImage& srcMIPImage,
        nvCompressionOptions* options,
        DXTWriteCallback fileWriteRoutine, // call to .dds write routine
        const RECT* rect = 0);

    DXTLIB_API NV_ERROR_CODE nvDXTcompress(const RGBAMipMappedCubeMap& srcMIPCubeMap,
        nvCompressionOptions* options,
        DXTWriteCallback fileWriteRoutine, // call to .dds write routine
        const RECT* rect = 0);

    DXTLIB_API NV_ERROR_CODE nvDXTcompress(const RGBAMipMappedVolumeMap& srcMIPVolumeMap,
        nvCompressionOptions* options,
        DXTWriteCallback fileWriteRoutine, // call to .dds write routine
        const RECT* rect = 0);
    
    DXTLIB_API NV_ERROR_CODE nvDXTcompress(const fpImage& srcImage,
        nvCompressionOptions* options,
        DXTWriteCallback fileWriteRoutine,
        const RECT* rect = 0);

    DXTLIB_API NV_ERROR_CODE nvDXTcompress(const fpMipMappedImage& srcMIPImage,
        nvCompressionOptions* options,
        DXTWriteCallback fileWriteRoutine,
        const RECT* rect = 0);

    DXTLIB_API NV_ERROR_CODE nvDXTcompress(const fpMipMappedCubeMap& srcMIPCubeMap,
        nvCompressionOptions* options,
        DXTWriteCallback fileWriteRoutine,  
        const RECT* rect = 0);

    DXTLIB_API NV_ERROR_CODE nvDXTcompress(const fpMipMappedVolumeMap& srcMIPVolumeMap,
        nvCompressionOptions* options,
        DXTWriteCallback fileWriteRoutine,  
        const RECT* rect = 0);
    
    // array of images for volume map or cube map
    DXTLIB_API NV_ERROR_CODE nvDXTcompress(const fpImageArray& srcImageArray,
        nvCompressionOptions* options,
        DXTWriteCallback fileWriteRoutine,  
        const RECT* rect = 0);
    
    // readMIPMapCount, number of MIP maps to load. 0 is all
    DXTLIB_API NV_ERROR_CODE nvDXTdecompress(
        nvImageContainer& imageData,
        nvPixelFormat pf,        
        int readMIPMapCount,
        DXTReadCallback fileReadRoutine,
        void* userData);
};
