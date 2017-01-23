///////////////////////////////////////////////////////////////////
// DESCRIPTION: 
//	Some utilities to do file format conversions and others ...
//
///////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "MNormalMapConverter.h"


// Convert the heightfield texture to its corresponding normal map texture
//
bool MNormalMapConverter::convertToNormalMap(
		unsigned char* inImagePtr,
		unsigned int width,
		unsigned int height,
		OutFormatType outputPixelFormat,
		float bumpScale,
		unsigned char* outImagePtr )
{
	bool	isOK = true;
	
	// Firewall: The input image should not be a NULL pointer,
	//
	if( NULL == inImagePtr )	return false;
	
	// No output file specified: convert in place
	//
	if( NULL == outImagePtr )
	{
		convertToNormalMap_InPlace( inImagePtr, width, height, outputPixelFormat, bumpScale );
	}
	else
	{
		if( outputPixelFormat == RGBA ) {
			//
			// Not implemented yet
			//
			// similar to the convertToNormalMap_InPlace but we store
			// the resulting texture in the outImagePtr and not
			// in the inImagePtr texture
			//
		}
		else if( outputPixelFormat == HILO ) {
			// Not implemented yet
		}
		else {
			isOK = false;
		}
	}

	return isOK;
}


// The heightfield texture inImage will be replaced from grey levels in RGBA
// to the normal map values as specified by the pixel format
//
bool MNormalMapConverter::convertToNormalMap_InPlace(
		unsigned char* inImagePtr,
		unsigned int width,
		unsigned int height,
		OutFormatType outputPixelFormat,
		float bumpScale )
{
	bool	isOK = true;

	if( outputPixelFormat == RGBA ) {

		bumpScale /= 255.0f;	// will be used on unsignedChar

		unsigned int widthMinus1  =  width - 1;
		unsigned int heightMinus1 = height - 1;
		unsigned int offset = (4 * width);	// = sizeof(rgba) * width

		// ==================
		// Process the texels
		// ==================

		// Get the current pointer to the starting texel at (0,0)
		//
		unsigned char* imagePtr = inImagePtr;

		// For each rows (except the last one)
		//
		unsigned int m, n;
		for( m=0; m<heightMinus1; m++ )
		{
			// Process the texel in each column of this row
			//
			for( n=0; n<widthMinus1; n++ )
			{
				float deltaU = bumpScale * (float)(imagePtr[0] - imagePtr[4]);
				float deltaV = bumpScale * (float)(imagePtr[0] - imagePtr[offset]);
				
				// Normalize (deltaU, deltaV, 1.)
				float sqlen = deltaU*deltaU + deltaV*deltaV + 1.0f;
				float rclen = 1.0f / _sqrt( sqlen );
				
				float nx = deltaU * rclen;
				float ny = deltaV * rclen;
				float nz = rclen;
				
				// Store the vector in red, green, blue
				*(imagePtr++) = (unsigned char) ((nx + 1.0f) * 127.5f); 
				*(imagePtr++) = (unsigned char) ((ny + 1.0f) * 127.5f); 
				*(imagePtr++) = (unsigned char) ((nz + 1.0f) * 127.5f); 
				
				// reset the alpha
				*(imagePtr++) = 255;
			}
			
			// At the end of the row, just copy the last value. One
			// could implement wraping instead.
			*(imagePtr++) = imagePtr[-4];
			*(imagePtr++) = imagePtr[-4];
			*(imagePtr++) = imagePtr[-4];
			*(imagePtr++) = 255;
		}

		// Fill in the last row (copy from the (last-1) row)
		// memcpy is the faster loop when memory is aligned.
		Memory.mem_copy(imagePtr, imagePtr-offset, width*4);
	}
	else if( outputPixelFormat == HILO ) {
		// Not implemented yet
	}
	else {
		isOK = false;
	}

	return isOK;
}


bool MNormalMapConverter::convertToHeightMap(
	unsigned char* inImagePtr,
	unsigned int width,
	unsigned int height,
	OutFormatType outputPixelFormat,
	float heightScale,
	unsigned char* outImagePtr)
{
	// Not implemented yet
	//
	return false;
}
