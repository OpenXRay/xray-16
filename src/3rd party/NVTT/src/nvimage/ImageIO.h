// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_IMAGEIO_H
#define NV_IMAGE_IMAGEIO_H

#include <nvimage/nvimage.h>

namespace nv
{
	class Image;
	class FloatImage;
	class Stream;

	namespace ImageIO
	{
		NVIMAGE_API Image * load(const char * fileName);
		NVIMAGE_API Image * load(const char * fileName, Stream & s);

		NVIMAGE_API FloatImage * loadFloat(const char * fileName);
		NVIMAGE_API FloatImage * loadFloat(const char * fileName, Stream & s);
		
		NVIMAGE_API bool save(const char * fileName, Stream & s, Image * img);
		NVIMAGE_API bool save(const char * fileName, Image * img);
		NVIMAGE_API bool saveFloat(const char * fileName, const FloatImage * fimage, uint base_component, uint num_components);

		NVIMAGE_API Image * loadTGA(Stream & s);
		NVIMAGE_API bool saveTGA(Stream & s, const Image * img);

		NVIMAGE_API Image * loadPSD(Stream & s);

#if defined(HAVE_PNG)
		NVIMAGE_API Image * loadPNG(Stream & s);
#endif

#if defined(HAVE_JPEG)
		NVIMAGE_API Image * loadJPG(Stream & s);
#endif

#if defined(HAVE_TIFF)
		NVIMAGE_API FloatImage * loadFloatTIFF(const char * fileName, Stream & s);
		
		NVIMAGE_API bool saveFloatTIFF(const char * fileName, const FloatImage * fimage, uint base_component, uint num_components);
#endif

#if defined(HAVE_OPENEXR)
		NVIMAGE_API FloatImage * loadFloatEXR(const char * fileName, Stream & s);
		
		NVIMAGE_API bool saveFloatEXR(const char * fileName, const FloatImage * fimage, uint base_component, uint num_components);
#endif

	//	NVIMAGE_API FloatImage * loadFloatPFM(const char * fileName, Stream & s);
	//	NVIMAGE_API bool saveFloatPFM(const char * fileName, const FloatImage * fimage, uint base_component, uint num_components);

	} // ImageIO namespace
	
} // nv namespace


#endif // NV_IMAGE_IMAGEIO_H
