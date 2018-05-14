// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_HOLEFILLING_H
#define NV_IMAGE_HOLEFILLING_H

#include <nvcore/BitArray.h>
#include <nvimage/nvimage.h>

namespace nv 
{
	class FloatImage;

	/// Bit mask.
	class BitMap
	{
	public:
		BitMap(uint w, uint h) : 
			m_width(w), m_height(h), m_bitArray(w*h) 
		{
		}
		
		const uint width() const { return m_width; }
		const uint height() const { return m_height; }
		
		bool bitAt(uint x, uint y) const
		{
			nvDebugCheck(x < m_width && y < m_height);
			return m_bitArray.bitAt(y * m_width + x);
		}
		bool bitAt(uint idx) const
		{
			return m_bitArray.bitAt(idx);
		}
	
		void setBitAt(uint x, uint y)
		{
			nvDebugCheck(x < m_width && y < m_height);
			m_bitArray.setBitAt(y * m_width + x);
		}
		void setBitAt(uint idx)
		{
			m_bitArray.setBitAt(idx);
		}
	
		void clearBitAt(uint x, uint y)
		{
			nvDebugCheck(x < m_width && y < m_height);
			m_bitArray.clearBitAt(y * m_width + x);
		}
		void clearBitAt(uint idx)
		{
			m_bitArray.clearBitAt(idx);
		}
	
		void clearAll()
		{
			m_bitArray.clearAll();
		}
	
		void setAll()
		{
			m_bitArray.setAll();
		}
	
		void toggleAll()
		{
			m_bitArray.toggleAll();
		}
		
		friend void swap(BitMap & a, BitMap & b)
		{
			nvCheck(a.m_width == b.m_width);
			nvCheck(a.m_height == b.m_height);
			//swap(const_cast<uint &>(a.m_width), const_cast<uint &>(b.m_width));
			//swap(const_cast<uint &>(a.m_height), const_cast<uint &>(b.m_height));
			swap(a.m_bitArray, b.m_bitArray);
		}
		
	private:
		
		const uint m_width;
		const uint m_height;
		BitArray m_bitArray;
		
	};

	NVIMAGE_API void fillVoronoi(FloatImage * img, const BitMap * bmap);
	NVIMAGE_API void fillBlur(FloatImage * img, const BitMap * bmap);
	NVIMAGE_API void fillPullPush(FloatImage * img, const BitMap * bmap);
	
	NVIMAGE_API void fillExtrapolate(int passCount, FloatImage * img, BitMap * bmap);
	NVIMAGE_API void fillQuadraticExtrapolate(int passCount, FloatImage * img, BitMap * bmap, int coverageIndex = -1);
	
} // nv namespace

#endif // NV_IMAGE_HOLEFILLING_H
