/*********************************************************************NVMH2****
Path:  D:\Dev\devrel\Nv_sdk_4\CommonSrc\nvImageLib
File:  NVI_Image.cpp

Copyright (C) 1999, 2000 NVIDIA Corporation
This file is provided without support, instruction, or implied warranty of any
kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
not liable under any circumstances for any damages or loss whatsoever arising
from the use or inability to use this file or items derived from it.

Comments:
General image data classes

class NVI_Image
class NVI_ImageBordered


******************************************************************************/
#include "stdafx.h"

#include "NVI_Image.h"

#include <limits.h>			// for UINT_MAX

using namespace xray_nvi;
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

NVI_Image::NVI_Image()
{
	m_pArray = NULL;
	m_Format = IMAGE_NOT_INITIALIZED;
	m_nSizeX = 0;
	m_nSizeY = 0;
}


NVI_Image::~NVI_Image()
{
	Free();
}

/////////////////////////////////////////////////////////////////


HRESULT NVI_Image::Free()
{
	SAFE_ARRAY_DELETE( m_pArray );

	m_pArray = NULL;
	m_Format = IMAGE_NOT_INITIALIZED;
	m_nSizeX = 0;
	m_nSizeY = 0;

	return( S_OK );
}



HRESULT NVI_Image::Initialize( int width, int height, NVI_PIXEL_FORMAT fmt )
{
	NVI_Image::Free();

	m_Format	= fmt;
	m_nSizeX	= width;
	m_nSizeY	= height;

	int bytes_per_pixel = GetBytesPerPixel();

	m_pArray	= new BYTE[ width * height * bytes_per_pixel ];
	VERIFY		(m_pArray);
	
	return( S_OK );
}

HRESULT NVI_Image::Initialize( int width, int height, NVI_PIXEL_FORMAT fmt, u8* data )
{
	NVI_Image::Initialize(width,height,fmt);

	int bytes_per_pixel = GetBytesPerPixel();
	CopyMemory(m_pArray,data,width * height * bytes_per_pixel);

	return( S_OK );
}


UINT	NVI_Image::GetNumPixels()
{
	return( GetHeight() * GetWidth() );
}



UINT	NVI_Image::GetBytesPerPixel()
{

	switch( m_Format )
	{
	case NVI_A8:
		return( 1 );
		break;

	case NVI_A8_R8_G8_B8:

		return( 4 );
		break;

	case NVI_A1_R5_G5_B5:
	case NVI_R5_G6_B5:
	case NVI_A16:

		return( 2 );
		break;

	case NVI_R16_G16_B16_A16:

		return( 8 );
		break;
	}

//.	FDebug("Unrecognized format! %d\n", m_Format );
	assert( false );
	return( 0 );
}


UINT	NVI_Image::GetImageNumBytes()
{
	#ifdef _DEBUG
		float numbytes;
		numbytes = (float) m_nSizeX * (float)m_nSizeY * (float)GetBytesPerPixel();
		assert( numbytes < ((float)UINT_MAX) );
	#endif


	return( m_nSizeX * m_nSizeY * GetBytesPerPixel() );
}


bool	NVI_Image::IsDataValid()
{
	bool res;
	res =			( m_pArray != NULL	);
	res = res &&	( m_nSizeX > 0		);
	res = res &&	( m_nSizeY > 0		);

	return( res );
}



void	NVI_Image::FlipTopToBottom()
{
	assert( IsDataValid() );

	UINT bpp = GetBytesPerPixel();
	assert( bpp >= 1 );

	UINT width = GetWidth();


	BYTE * swap = new BYTE[ width * bpp ];
	assert( swap != NULL );

	DWORD row;
	UINT height = GetHeight();
	BYTE * end_row;
	BYTE * start_row;

	for( row = 0; row < GetHeight() / 2; row ++ )
	{
		end_row =   & (m_pArray[ bpp*width*( height - row - 1) ]);
		start_row = & (m_pArray[ bpp * width * row ]);

		// copy row toward end of image into temporary swap buffer
		memcpy( swap, end_row, bpp * width );

		// copy row at beginning to row at end
		memcpy( end_row, start_row, bpp * width );

		// copy old bytes from row at end (in swap) to row at beginning
		memcpy( start_row, swap, bpp * width );
	}

	SAFE_ARRAY_DELETE( swap );
}





void	NVI_Image::AverageRGBToAlpha()
{
	// write each pixels' avg r,g,b to alpha

	assert( IsDataValid() );

	// Only implemented for a8r8g8b8 images so far
	assert( GetFormat() == NVI_A8_R8_G8_B8 );


	int k,cnt=m_nSizeY*m_nSizeX;
	DWORD pix;
	float r,g,b;

	////////////////////////////
	// a8r8g8b8 implementation

	for ( k = 0; k < cnt; k++  ){
		GetPixel_ARGB8( & pix, k );

		r = (float) (( pix & 0x00FF0000 ) >> 16	);
		g = (float) (( pix & 0x0000FF00 ) >> 8	);
		b = (float) (( pix & 0x000000FF )		);

		r = ( r + g + b ) / 3.0f;

		pix &= 0x00FFFFFF;
		pix |= ((DWORD)r) << 24;

		SetPixel_ARGB8( k, pix );
	}
}



void	NVI_Image::ABGR8_To_ARGB8()
{
	// swaps RGB for all pixels

	assert( IsDataValid() );
	assert( GetBytesPerPixel() == 4 );


	UINT  hxw = GetNumPixels();
	UINT  i;
	DWORD col, a,r,g,b;

	for( i=0; i < hxw; i++ )
	{
		GetPixel_ARGB8( &col, i );

		a = ( col >> 24 ) && 0x000000FF;
		b = ( col >> 16 ) && 0x000000FF;
		g = ( col >> 8  ) && 0x000000FF;
		r = ( col >> 0  ) && 0x000000FF;

		col = ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b ;

		SetPixel_ARGB8( i, col );
	}
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


NVI_ImageBordered::NVI_ImageBordered()
: NVI_Image()
{
	m_hSrcImage = 0;
	m_nBorderXHigh = 0;
	m_nBorderYHigh = 0;
	m_nBorderXLow = 0;
	m_nBorderYLow = 0;
	m_bWrap = 0;

	m_pArray = NULL;

}

NVI_ImageBordered::~NVI_ImageBordered()
{
	Free();
}


HRESULT NVI_ImageBordered::Free()
{
	NVI_Image::Free();

	m_hSrcImage = 0;
	m_nBorderXHigh = 0;
	m_nBorderYHigh = 0;
	m_nBorderXLow = 0;
	m_nBorderYLow = 0;
	m_bWrap = 0;

	return( S_OK );
}


HRESULT NVI_ImageBordered::Initialize( NVI_Image ** hSrcImage, const RECT * pBorder, bool wrap )
{
	assert( hSrcImage != NULL );
	assert( *hSrcImage != NULL );
	assert( pBorder != NULL );

	assert( pBorder->left <= 0 );
	assert( pBorder->bottom <= 0 );
	assert( pBorder->right >= 0 );
	assert( pBorder->top >= 0 );

	// only support 32-bit ARGB for now...
	assert( (*hSrcImage)->m_Format == NVI_A8_R8_G8_B8 );

	Free();

	m_hSrcImage = hSrcImage;

	m_nBorderXLow	= pBorder->left;
	m_nBorderXHigh	= pBorder->right;
	m_nBorderYLow	= pBorder->bottom;
	m_nBorderYHigh	= pBorder->top;

	// m_nBorderXLow <= 0
	m_nSizeX = (*hSrcImage)->m_nSizeX - m_nBorderXLow + m_nBorderXHigh;
	m_nSizeY = (*hSrcImage)->m_nSizeY - m_nBorderYLow + m_nBorderYHigh;



	NVI_Image::Initialize( m_nSizeX, m_nSizeY, (*m_hSrcImage)->m_Format );


	//  Now copy the source bits & pad out the edges
	m_bWrap = wrap;

	
	CopyDataFromSource();

	return( S_OK );
}


void	NVI_ImageBordered::CopyDataFromSource()
{
	assert( m_pArray );
	assert( m_hSrcImage );
	assert( *m_hSrcImage );

	assert( m_Format == NVI_A8_R8_G8_B8 );


	int sx, sy;		// size of image array

	sx = (*m_hSrcImage)->m_nSizeX;
	sy = (*m_hSrcImage)->m_nSizeY;


	DWORD * pSrcArray = (DWORD*) (*m_hSrcImage)->m_pArray;
	assert( pSrcArray != NULL );

	DWORD * pPadArray = (DWORD*) m_pArray;
	assert( m_pArray != NULL );

	int i,j;

	// Duplicate values into the padded region, taking wrapping into account
	
//.	ASSERT_MSG( m_nBorderXLow < sx, "Borders larger than image not supported! ulow\n" );
//.	ASSERT_MSG( m_nBorderYLow < sy, "Borders larger than image not supported! uhigh\n" );
//.	ASSERT_MSG( m_nBorderXHigh < sx, "Borders larger than image not supported! vlow\n" );
//.	ASSERT_MSG( m_nBorderYHigh < sy, "Borders larger than image not supported! vhigh\n" );

	// First, copy source image within the borders

	for( j=0; j < sy; j++ )
	{	
		// j is in coords of the source image
		// low bounds are negative or zero
	    memcpy( & pPadArray[ (j-m_nBorderYLow) * m_nSizeX - m_nBorderXLow ],
				& pSrcArray[ j*sx ], sizeof( DWORD ) * sx );
	}


	if( m_bWrap )
	{
		// copy rows opposite
		// copy out values in x first

		for( j = 0; j < m_nSizeY; j++ )
		{
			// j is in coords of the dest padded array

			// Copy right side image pixels into left edge border padded area
			// Use (- low bound) as the low will be <= 0 
			memcpy( & pPadArray[ j*m_nSizeX ],
					& pPadArray[ (j*m_nSizeX) + sx ], sizeof( DWORD ) * (-m_nBorderXLow) );

			// Copy left side image pixels into right edge border padded area
			memcpy( & pPadArray[ j*m_nSizeX - m_nBorderXLow + sx ],
					& pPadArray[ j*m_nSizeX - m_nBorderXLow      ], sizeof(DWORD) * ( m_nBorderXHigh ) );
		}

		for( j = 0; j < m_nBorderYHigh; j++ )
		{
			// Copy low source image pixels into upper edge border padded area
			// krn_v_lowbound is negative or zero

			memcpy( &pPadArray[ (j + sy - m_nBorderYLow ) * m_nSizeX ],
					&pPadArray[ (j - m_nBorderYLow)       * m_nSizeX ], sizeof(DWORD) * m_nSizeX );
		}


		for( j = 0; j < -m_nBorderYLow; j++ )
		{
			// Copy high source image pixels into lower border padded area
			// krn_v_lowbound is negative or zero
			// This completes the image tiling into the larger padded texture			

			memcpy( &pPadArray[  j * m_nSizeX ],
					&pPadArray[ (j + sy-1) * m_nSizeX ], sizeof(DWORD) * m_nSizeX );
		}


	}
	else
	{
		// If not wrap

		// Duplicate border outward as though image texture were clamped at edges

		for( j = 0; j < m_nBorderYHigh; j++ )
		{
			// Copy highest source image pixel row into upper edge border padded area
			// krn_v_lowbound is negative or zero

			memcpy( &pPadArray[ (j + sy - m_nBorderYLow ) * m_nSizeX ],
					&pPadArray[ (sy - 1 - m_nBorderYLow ) * m_nSizeX ], sizeof(DWORD) * m_nSizeX );
		}


		for( j = 0; j < -m_nBorderYLow; j++ )
		{
			// Copy lowest source image pixels into lower border padded area
			// krn_v_lowbound is negative or zero
			// This completes the image tiling into the larger padded texture			

			memcpy( &pPadArray[  j * m_nSizeX ],
					&pPadArray[ (- m_nBorderYLow) * m_nSizeX ], sizeof(DWORD) * m_nSizeX );
		}

		// Now copy out border pixels to left and right

		for( j = 0; j < m_nSizeY; j++ )
		{
			// j is in coords of the dest padded array

			// Copy right side image pixel to fill the right side padded row
			for( i = sx - m_nBorderXLow; i < m_nSizeX; i++ )
			{
				memcpy( & pPadArray[ j*m_nSizeX + i    ],
						& pPadArray[ j*m_nSizeX + i - 1], sizeof( DWORD ) );
			}

			// Copy left side src image pixel into left edge padded area

			for( i = -m_nBorderXLow - 1; i >= 0; i-- )
			{
				memcpy( & pPadArray[ j*m_nSizeX + i    ],
						& pPadArray[ j*m_nSizeX + i + 1], sizeof( DWORD ) );
			}
		}
	}


	// To save the result of the padding operation:
	//	ulTarga newfile;
	//	newfile.WriteFile( "temp_result1.tga", (unsigned char*) pPadArray,
	//						(DWORD) m_nSizeX, (DWORD) m_nSizeY, 32, 32, 0 );

}
