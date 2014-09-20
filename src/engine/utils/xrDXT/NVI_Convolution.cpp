/*********************************************************************NVMH2****
Path:  C:\Dev\devrel\Nv_sdk_4\CommonSrc\nvImageLib
File:  NVI_Convolution.cpp

Copyright (C) 1999, 2000 NVIDIA Corporation
This file is provided without support, instruction, or implied warranty of any
kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
not liable under any circumstances for any damages or loss whatsoever arising
from the use or inability to use this file or items derived from it.

Comments:


******************************************************************************/
#include "stdafx.h"


#include "NVI_Convolution.h"

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

using namespace xray_nvi;

ConvolutionKernel::ConvolutionKernel()
{
	m_pElements = NULL;
	m_nNumElements = 0;
}


ConvolutionKernel::~ConvolutionKernel()
{
	Free();
}

//////////////////////////////////////////////////////////

ConvolutionKernel & ConvolutionKernel::operator = ( const ConvolutionKernel & src )
{
	Initialize( src.m_nNumElements );

	int n;
	for( n=0; n < src.m_nNumElements; n++ )
	{
		m_pElements[n] = src.m_pElements[n];
	}

	return( *this );
}

///////////////////////////////////////////////////////////



HRESULT	ConvolutionKernel::Initialize( int num_elements )
{
	Free();

	m_pElements = new ConvolutionKernelElement[ num_elements ];

	assert( m_pElements != NULL );

	m_nNumElements = num_elements;

	return( S_OK );
}


HRESULT	ConvolutionKernel::Free()
{
	if( m_pElements != NULL )
	{
		delete[]	m_pElements;
		m_pElements = NULL;
	}

	m_nNumElements = 0;

	return( S_OK );
}



void	ConvolutionKernel::SetElements( int num_elements, ConvolutionKernelElement * pElements )
{
	HRESULT hr;
	int i;

	assert( pElements != NULL );


	if( num_elements != m_nNumElements )
	{
		hr = Initialize( num_elements );
		ASSERT_IF_FAILED(hr);
	}

	
	for( i=0; i < m_nNumElements; i++ )
	{
		m_pElements[i].x_offset = pElements[i].x_offset;
		m_pElements[i].y_offset = pElements[i].y_offset;
		m_pElements[i].weight   = pElements[i].weight;
	}

}



//===============================================================
// Function:	ConvolutionKernel::GetKernelExtents
// Description:	Querries the kernel elements & finds out what area they 
//				extend over.
// Parameters:	Pointers to values to be written
// Returns:		Return values written to pointer addresses
//===============================================================

void ConvolutionKernel::GetKernelExtents( int * xlow, int * xhigh, int * ylow, int * yhigh )
{
	// querries the kernel elements & finds out what area they 
	//  extend over.
	// Return values written to pointer addresses

	
//.	ASSERT_MSG( m_pElements, "Initialize the m_pElements pointer!!\n");
//.	ASSERT_MSG( m_nNumElements > 0, "Kernel has no elements!\n");
//.	ASSERT_MSG( ((xlow!=NULL)&&(ylow!=NULL)&&(xhigh!=NULL)&&(yhigh!=NULL)), "Input pointer is null!\n");

	*xlow = *xhigh = m_pElements[0].x_offset;
	*ylow = *yhigh = m_pElements[0].y_offset;

	int i;
	for( i=1; i < m_nNumElements; i++ )
	{
		if( m_pElements[i].x_offset < *xlow )
			*xlow = m_pElements[i].x_offset;
		if( m_pElements[i].x_offset > *xhigh )
			*xhigh = m_pElements[i].x_offset;

		if( m_pElements[i].y_offset < *ylow )
			*ylow = m_pElements[i].y_offset;
		if( m_pElements[i].y_offset > *yhigh )
			*yhigh = m_pElements[i].y_offset;
	}
}



//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////


Convolver::Convolver()
: m_BorderedImage()
{
	m_hSrcImage = NULL;
	m_pKernels = NULL;
	m_nNumKernels = NULL;
}


Convolver::~Convolver()
{
	Free();
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////


HRESULT Convolver::Free()
{
	SAFE_ARRAY_DELETE( m_pKernels );
	m_BorderedImage.Free();

	m_hSrcImage = NULL;
	m_pKernels = NULL;
	m_nNumKernels = NULL;

	return( S_OK );
}


HRESULT Convolver::Initialize( NVI_Image ** hSrcImage, const ConvolutionKernel * pKernels,
								int num_kernels, bool wrap )
{
	assert( hSrcImage != NULL );
	assert( *hSrcImage != NULL );
	assert( pKernels != NULL );
	assert( num_kernels > 0 );

	RECT max_kern_size;
	int xlow, xhigh, ylow, yhigh; 

	m_hSrcImage = hSrcImage;

	m_nNumKernels = num_kernels;

	m_pKernels = new ConvolutionKernel[ m_nNumKernels ];
	assert( m_pKernels != NULL );

	int n;

	max_kern_size.bottom	= 0;
	max_kern_size.top		= 0;
	max_kern_size.left		= 0;
	max_kern_size.right		= 0;

	for( n=0; n < m_nNumKernels; n++ )
	{
		m_pKernels[n] = pKernels[n];

		m_pKernels[n].GetKernelExtents( &xlow, &xhigh, &ylow, &yhigh );
		
		if( xlow < max_kern_size.left )
			max_kern_size.left = xlow;
		if( ylow < max_kern_size.bottom )
			max_kern_size.bottom = ylow;
		if( xhigh > max_kern_size.right )
			max_kern_size.right = xhigh;
		if( yhigh > max_kern_size.top )
			max_kern_size.top = yhigh;
	}

	// Initialize allocated the bordered image & copies from source
	m_BorderedImage.Initialize( hSrcImage, & max_kern_size, wrap );


	return( S_OK );
}


void	Convolver::Convolve_Alpha_At( int i, int j, float * results, int num_results )
{
	assert( num_results == m_nNumKernels );
	assert( m_BorderedImage.m_pArray != NULL );

	int n, nkern;

	float height;
	float result;
	DWORD color;
	float byte_to_float = 1.0f / 255.0f;

	for( nkern = 0; nkern < m_nNumKernels; nkern++ )
	{
		result = 0;

		for( n=0; n < m_pKernels[nkern].m_nNumElements; n++ )
		{
			// extract alpha
			m_BorderedImage.GetPixel( &color,
								i + m_pKernels[nkern].m_pElements[n].x_offset,
								j + m_pKernels[nkern].m_pElements[n].y_offset  );

			// byte_to_float takes 0,255 to 0,1
			height = (float) ( color >> 24 ) * byte_to_float;

			height *= m_pKernels[nkern].m_pElements[n].weight ;
		
			result += height;
		}
		results[nkern] = result;
	}
}
