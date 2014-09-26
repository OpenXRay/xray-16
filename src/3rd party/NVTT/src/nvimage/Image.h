// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_IMAGE_H
#define NV_IMAGE_IMAGE_H

#include <nvcore/Debug.h>
#include <nvimage/nvimage.h>

namespace nv
{
	class Color32;
	
	/// 32 bit RGBA image.
	class NVIMAGE_CLASS Image
	{
	public:
		
		enum Format 
		{
			Format_RGB,
			Format_ARGB,
		};
		
		Image();
		Image(const Image & img);
		~Image();

		const Image & operator=(const Image & img);


		void allocate(uint w, uint h);
		bool load(const char * name);
		
		void wrap(void * data, uint w, uint h);
		void unwrap();
		
		uint width() const;
		uint height() const;
		
		const Color32 * scanline(uint h) const;
		Color32 * scanline(uint h);
		
		const Color32 * pixels() const;
		Color32 * pixels();
		
		const Color32 & pixel(uint idx) const;
		Color32 & pixel(uint idx);
		
		const Color32 & pixel(uint x, uint y) const;
		Color32 & pixel(uint x, uint y);
		
		Format format() const;
		void setFormat(Format f);
		
		void fill(Color32 c);

	private:
		void free();
		
	private:
		uint m_width;
		uint m_height;
		Format m_format;
		Color32 * m_data;
	};


	inline const Color32 & Image::pixel(uint x, uint y) const
	{
		nvDebugCheck(x < width() && y < height());
		return pixel(y * width() + x);
	}
	
	inline Color32 & Image::pixel(uint x, uint y)
	{
		nvDebugCheck(x < width() && y < height());
		return pixel(y * width() + x);
	}

} // nv namespace


#endif // NV_IMAGE_IMAGE_H
