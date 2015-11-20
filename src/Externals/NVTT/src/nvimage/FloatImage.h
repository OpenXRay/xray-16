// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_FLOATIMAGE_H
#define NV_IMAGE_FLOATIMAGE_H

#include <nvimage/nvimage.h>

#include <nvmath/Vector.h>

#include <nvcore/Debug.h>
#include <nvcore/Containers.h> // clamp

#include <stdlib.h> // abs


namespace nv
{
class Vector4;
class Matrix;
class Image;
class Filter;
class Kernel1;
class Kernel2;
class PolyphaseKernel;

/// Multicomponent floating point image class.
class FloatImage
{
public:

	enum WrapMode {
		WrapMode_Clamp,
		WrapMode_Repeat,
		WrapMode_Mirror
	};
	
	NVIMAGE_API FloatImage();
	NVIMAGE_API FloatImage(const Image * img);
	NVIMAGE_API virtual ~FloatImage();

	/** @name Conversion. */
	//@{
	NVIMAGE_API void initFrom(const Image * img);
	NVIMAGE_API Image * createImage(uint base_component = 0, uint num = 4) const;
	NVIMAGE_API Image * createImageGammaCorrect(float gamma = 2.2f) const;
	//@}

	/** @name Allocation. */
	//@{
	NVIMAGE_API void allocate(uint c, uint w, uint h);
	NVIMAGE_API void free(); // Does not clear members.
	//@}

	/** @name Manipulation. */
	//@{
	NVIMAGE_API void clear(float f=0.0f);

	NVIMAGE_API void normalize(uint base_component);
	
	NVIMAGE_API void packNormals(uint base_component);
	NVIMAGE_API void expandNormals(uint base_component);
	NVIMAGE_API void scaleBias(uint base_component, uint num, float scale, float add);
	
	//NVIMAGE_API void clamp(uint base_component, uint num);
	NVIMAGE_API void clamp(float low, float high);
	
	NVIMAGE_API void toLinear(uint base_component, uint num, float gamma = 2.2f);
	NVIMAGE_API void toGamma(uint base_component, uint num, float gamma = 2.2f);
	NVIMAGE_API void exponentiate(uint base_component, uint num, float power);
	

	NVIMAGE_API FloatImage * fastDownSample() const;
	NVIMAGE_API FloatImage * downSample(const Filter & filter, WrapMode wm) const;
	NVIMAGE_API FloatImage * downSample(const Filter & filter, WrapMode wm, uint alpha) const;
	NVIMAGE_API FloatImage * resize(const Filter & filter, uint w, uint h, WrapMode wm) const;

	NVIMAGE_API FloatImage * resize(const Filter & filter, uint w, uint h, WrapMode wm, uint alpha) const;
	//@}

	NVIMAGE_API float applyKernel(const Kernel2 * k, int x, int y, uint c, WrapMode wm) const;
	NVIMAGE_API float applyKernelVertical(const Kernel1 * k, int x, int y, uint c, WrapMode wm) const;
	NVIMAGE_API float applyKernelHorizontal(const Kernel1 * k, int x, int y, uint c, WrapMode wm) const;
	NVIMAGE_API void applyKernelVertical(const PolyphaseKernel & k, int x, uint c, WrapMode wm, float * output) const;
	NVIMAGE_API void applyKernelHorizontal(const PolyphaseKernel & k, int y, uint c, WrapMode wm, float * output) const;
	NVIMAGE_API void applyKernelVertical(const PolyphaseKernel & k, int x, uint c, uint a, WrapMode wm, float * output) const;
	NVIMAGE_API void applyKernelHorizontal(const PolyphaseKernel & k, int y, uint c, uint a, WrapMode wm, float * output) const;
	
	
	uint width() const { return m_width; }
	uint height() const { return m_height; }
	uint componentNum() const { return m_componentNum; }
	uint count() const { return m_count; }


	/** @name Pixel access. */
	//@{
	const float * channel(uint c) const;
	float * channel(uint c);
	
	const float * scanline(uint y, uint c) const;
	float * scanline(uint y, uint c);
	
	void setPixel(float f, uint x, uint y, uint c);
	void addPixel(float f, uint x, uint y, uint c);
	float pixel(uint x, uint y, uint c) const;
	
	void setPixel(float f, uint idx);
	float pixel(uint idx) const;
	
	float sampleNearest(float x, float y, int c, WrapMode wm) const;
	float sampleLinear(float x, float y, int c, WrapMode wm) const;
	
	float sampleNearestClamp(float x, float y, int c) const;
	float sampleNearestRepeat(float x, float y, int c) const;
	float sampleNearestMirror(float x, float y, int c) const;
	
	float sampleLinearClamp(float x, float y, int c) const;
	float sampleLinearRepeat(float x, float y, int c) const;
	float sampleLinearMirror(float x, float y, int c) const;
	//@}
	
	
	FloatImage* clone() const;
	
public:
	
	uint index(uint x, uint y) const;
	uint indexClamp(int x, int y) const;
	uint indexRepeat(int x, int y) const;
	uint indexMirror(int x, int y) const;
	uint index(int x, int y, WrapMode wm) const;

public:

	uint16 m_width;			///< Width of the texture.
	uint16 m_height;		///< Height of the texture.
	uint32 m_componentNum;	///< Number of components.
	uint32 m_count;			///< Image pixel count.
	float * m_mem;

};


/// Get const channel pointer.
inline const float * FloatImage::channel(uint c) const
{
	nvDebugCheck(m_mem != NULL);
	nvDebugCheck(c < m_componentNum);
	return m_mem + c * m_width * m_height;
}

/// Get channel pointer.
inline float * FloatImage::channel(uint c) {
	nvDebugCheck(m_mem != NULL);
	nvDebugCheck(c < m_componentNum);
	return m_mem + c * m_width * m_height;
}

/// Get const scanline pointer.
inline const float * FloatImage::scanline(uint y, uint c) const
{
	nvDebugCheck(y < m_height);
	return channel(c) + y * m_width;
}

/// Get scanline pointer.
inline float * FloatImage::scanline(uint y, uint c)
{
	nvDebugCheck(y < m_height);
	return channel(c) + y * m_width;
}

/// Set pixel component.
inline void FloatImage::setPixel(float f, uint x, uint y, uint c)
{
	nvDebugCheck(m_mem != NULL);
	nvDebugCheck(x < m_width);
	nvDebugCheck(y < m_height);
	nvDebugCheck(c < m_componentNum);
	m_mem[(c * m_height + y) * m_width + x] = f;
}

/// Add to pixel component.
inline void FloatImage::addPixel(float f, uint x, uint y, uint c)
{
	nvDebugCheck(m_mem != NULL);
	nvDebugCheck(x < m_width);
	nvDebugCheck(y < m_height);
	nvDebugCheck(c < m_componentNum);
	m_mem[(c * m_height + y) * m_width + x] += f;
}

/// Get pixel component.
inline float FloatImage::pixel(uint x, uint y, uint c) const
{
	nvDebugCheck(m_mem != NULL);
	nvDebugCheck(x < m_width);
	nvDebugCheck(y < m_height);
	nvDebugCheck(c < m_componentNum);
	return m_mem[(c * m_height + y) * m_width + x];
}

/// Set pixel component.
inline void FloatImage::setPixel(float f, uint idx)
{
	nvDebugCheck(idx < m_count);
	m_mem[idx] = f;
}

/// Get pixel component.
inline float FloatImage::pixel(uint idx) const
{
	nvDebugCheck(idx < m_count);
	return m_mem[idx];
}

inline uint FloatImage::index(uint x, uint y) const
{
	nvDebugCheck(x < m_width);
	nvDebugCheck(y < m_height);
	return y * m_width + x;
}

inline uint FloatImage::indexClamp(int x, int y) const
{
	return nv::clamp(y, int(0), int(m_height-1)) * m_width + nv::clamp(x, int(0), int(m_width-1));
}

inline int repeat_remainder(int a, int b)
{
   if (a >= 0) return a % b;
   else return (a + 1) % b + b - 1;
}

inline uint FloatImage::indexRepeat(int x, int y) const
{
	return repeat_remainder(y, m_height) * m_width + repeat_remainder(x, m_width);
}

inline uint FloatImage::indexMirror(int x, int y) const
{
	if (m_width == 1) x = 0;

	x = abs(x);
	while (x >= m_width) {
		x = abs(m_width + m_width - x - 2);
	}

	if (m_height == 1) y = 0;

	y = abs(y);
	while (y >= m_height) {
		y = abs(m_height + m_height - y - 2);
	}

	return index(x, y);
}

inline uint FloatImage::index(int x, int y, WrapMode wm) const
{
	if (wm == WrapMode_Clamp) return indexClamp(x, y);
	if (wm == WrapMode_Repeat) return indexRepeat(x, y);
	/*if (wm == WrapMode_Mirror)*/ return indexMirror(x, y);
}

} // nv namespace



#endif // NV_IMAGE_FLOATIMAGE_H
