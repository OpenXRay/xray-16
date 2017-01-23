/*********************************************************************NVMH2****
Path:  C:\Dev\devrel\Nv_sdk_4\CommonSrc\nvImageLib
File:  NVI_Convolution.h

Copyright (C) 1999, 2000 NVIDIA Corporation
This file is provided without support, instruction, or implied warranty of any
kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
not liable under any circumstances for any damages or loss whatsoever arising
from the use or inability to use this file or items derived from it.

Comments:

******************************************************************************/

#ifndef __NVIMAGELIB_CONVOLUTION_H
#define __NVIMAGELIB_CONVOLUTION_H

#include "NVI_Image.h"

namespace xray_nvi
{
    struct ConvolutionKernelElement
    {
        int	x_offset; // Coordinates of sample point
        int y_offset; //   relative to center
        float weight; // Weight to multiply sample point by
    };

    // Kernel with arbitrary sample placement - 
    // Doesn't have to be square or evenly distributed
    class ConvolutionKernel
    {
    public:
        ConvolutionKernelElement* m_pElements;
        int m_nNumElements;
        ConvolutionKernel();
        ~ConvolutionKernel();
        HRESULT Initialize(int numElements);
        HRESULT Free();
        void SetElements(int numElements, ConvolutionKernelElement* pElements);
        // Find extent (rectangle) over which the kernel samples
        // Values are the offset from the (0,0) element
        void GetKernelExtents(int* xlow, int* xhigh, int* ylow, int* yhigh);
        ConvolutionKernel& operator=(const ConvolutionKernel& src);
    };

    // A class to drive convolutions.
    // Currently only supports A8R8G8B8 inputs
    class Convolver
    {
    private:
        NVI_Image** m_hSrcImage;
        NVI_ImageBordered m_BorderedImage;
        ConvolutionKernel* m_pKernels;
        int m_nNumKernels;

    public:
        Convolver();
        ~Convolver();
        HRESULT Initialize(NVI_Image** pSrcImage, const ConvolutionKernel* pKernels, int numKernels, bool wrap);
        HRESULT Free();
        // Coords in source image
        // numResults must equal numKernels set on Initialize();
        void Convolve_Alpha_At(int i, int j, float* results, int numResults);
    };
};

#endif			// __NVIMAGELIB_CONVOLUTION_H
