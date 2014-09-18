#ifndef MAYA_API_MNormalMapConverter
#define MAYA_API_MNormalMapConverter


// MNormalMapConverter.h
//
// Copyright (C) 2000-2003 Alias|Wavefront, a division of Silicon Graphics Limited.
// 
// The information in this file is provided for the exclusive use of the
// licensees of Alias|Wavefront.  Such users have the right to use, modify,
// and incorporate this code into other products for purposes authorized
// by the Alias|Wavefront license agreement, without fee.
// 
// ALIAS|WAVEFRONT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
// INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
// EVENT SHALL ALIAS|WAVEFRONT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
// CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
// DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
// TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

class MNormalMapConverter
{
public:
	enum OutFormatType
	{
		RGBA,
		HILO
	};

	MNormalMapConverter(){};
	~MNormalMapConverter(){};

	// These functions assume the that file texture is square, and
	// that its dimensions are exponents of 2.
	//

	// Convert the heightfield texture to its corresponding normal map texture
	//
	bool convertToNormalMap(
		unsigned char* inImagePtr,
		unsigned int width,
		unsigned int height,
		OutFormatType outputPixelFormat,
		float bumpScale = 1.0,
		unsigned char* outImagePtr = NULL );

	// Convert the normal map texture to its corresponding heightfield texture
	//
	bool convertToHeightMap(
		unsigned char* inImagePtr,
		unsigned int width,
		unsigned int height,
		OutFormatType outputPixelFormat,
		float heightScale = 1.0,
		unsigned char* outImagePtr = NULL );

protected:

	// Convert the heightfield to the normal map texture in place
	// (replace the input texture with the normal map version)
	//
	bool convertToNormalMap_InPlace(
		unsigned char* inImagePtr,
		unsigned int width,
		unsigned int height,
		OutFormatType outputPixelFormat,
		float bumpScale );
};

#endif // MAYA_API_MNormalMapConverter
