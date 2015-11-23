// This code is in the public domain -- castanyo@yahoo.es

/** @file Filter.cpp
 * @brief Image filters.
 *
 * Jonathan Blow articles:
 * http://number-none.com/product/Mipmapping, Part 1/index.html
 * http://number-none.com/product/Mipmapping, Part 2/index.html
 *
 * References from Thacher Ulrich:
 * See _Graphics Gems III_ "General Filtered Image Rescaling", Dale A. Schumacher
 * http://tog.acm.org/GraphicsGems/gemsiii/filter.c
 *
 * References from Paul Heckbert:
 * A.V. Oppenheim, R.W. Schafer, Digital Signal Processing, Prentice-Hall, 1975
 *
 * R.W. Hamming, Digital Filters, Prentice-Hall, Englewood Cliffs, NJ, 1983
 *
 * W.K. Pratt, Digital Image Processing, John Wiley and Sons, 1978
 *
 * H.S. Hou, H.C. Andrews, "Cubic Splines for Image Interpolation and
 *	Digital Filtering", IEEE Trans. Acoustics, Speech, and Signal Proc.,
 *	vol. ASSP-26, no. 6, Dec. 1978, pp. 508-517
 *
 * Paul Heckbert's zoom library.
 * http://www.xmission.com/~legalize/zoom.html
 * 
 * Reconstruction Filters in Computer Graphics
 * http://www.mentallandscape.com/Papers_siggraph88.pdf
 *
 * More references:
 * http://www.worldserver.com/turk/computergraphics/ResamplingFilters.pdf
 * http://www.dspguide.com/ch16.htm
 */

#include "Filter.h"

#include <nvmath/Vector.h>	// Vector4
#include <nvcore/Containers.h>	// swap

using namespace nv;

namespace
{
	// Sinc function.
	inline static float sincf(const float x)
	{
		if (fabs(x) < NV_EPSILON) {
			//return 1.0;
			return 1.0f + x*x*(-1.0f/6.0f + x*x*1.0f/120.0f);
		}
		else {
			return sin(x) / x;
		}
	}

	// Bessel function of the first kind from Jon Blow's article.
	// http://mathworld.wolfram.com/BesselFunctionoftheFirstKind.html
	// http://en.wikipedia.org/wiki/Bessel_function
	inline static float bessel0(float x)
	{
		const float EPSILON_RATIO = 1e-6f;
		float xh, sum, pow, ds;
		int k;

		xh = 0.5f * x;
		sum = 1.0f;
		pow = 1.0f;
		k = 0;
		ds = 1.0;
		while (ds > sum * EPSILON_RATIO) {
			++k;
			pow = pow * (xh / k);
			ds = pow * pow;
			sum = sum + ds;
		}

		return sum;
	}

	/*// Alternative bessel function from Paul Heckbert.
	static float _bessel0(float x)
	{
		const float EPSILON_RATIO = 1E-6;
		float sum = 1.0f;
		float y = x * x / 4.0f;
		float t = y;
		for(int i = 2; t > EPSILON_RATIO; i++) {
			sum += t;
			t *= y / float(i * i);
		}
		return sum;
	}*/

} // namespace


Filter::Filter(float width) : m_width(width)
{
}

/*virtual*/ Filter::~Filter()
{
}

float Filter::sampleDelta(float x, float scale) const
{
	return evaluate((x + 0.5f)* scale);
}

float Filter::sampleBox(float x, float scale, int samples) const
{
	float sum = 0;
	float isamples = 1.0f / float(samples);

	for(int s = 0; s < samples; s++)
	{
		float p = (x + (float(s) + 0.5f) * isamples) * scale;
		float value = evaluate(p);
		sum += value;
	}
	
	return sum * isamples;
}

float Filter::sampleTriangle(float x, float scale, int samples) const
{
	float sum = 0;
	float isamples = 1.0f / float(samples);

	for(int s = 0; s < samples; s++)
	{
		float offset = (2 * float(s) + 1.0f) * isamples;		
		float p = (x + offset - 0.5f) * scale;
		float value = evaluate(p);
		
		float weight = offset;
		if (weight > 1.0f) weight = 2.0f - weight;
		
		sum += value * weight;
	}
	
	return 2 * sum * isamples;
}





BoxFilter::BoxFilter() : Filter(0.5f) {}
BoxFilter::BoxFilter(float width) : Filter(width) {}

float BoxFilter::evaluate(float x) const
{
	if (fabs(x) <= m_width) return 1.0f;
	else return 0.0f;
}


TriangleFilter::TriangleFilter() : Filter(1.0f) {}
TriangleFilter::TriangleFilter(float width) : Filter(width) {}

float TriangleFilter::evaluate(float x) const
{
	x = fabs(x);
    if( x < m_width ) return m_width - x;
    return 0.0f;
}


QuadraticFilter::QuadraticFilter() : Filter(1.5f) {}

float QuadraticFilter::evaluate(float x) const
{
	x = fabs(x);
    if( x < 0.5f ) return 0.75f - x * x;
    if( x < 1.5f ) { 
    	float t = x - 1.5f;
    	return 0.5f * t * t;
    }
    return 0.0f;
}


CubicFilter::CubicFilter() : Filter(1.0f) {}

float CubicFilter::evaluate(float x) const
{
	// f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1
	x = fabs(x);
	if( x < 1.0f ) return((2.0f * x - 3.0f) * x * x + 1.0f);
	return 0.0f;
}


BSplineFilter::BSplineFilter() : Filter(2.0f) {}

float BSplineFilter::evaluate(float x) const
{
	x = fabs(x);
    if( x < 1.0f ) return (4.0f + x * x * (-6.0f + x * 3.0f)) / 6.0f;
    if( x < 2.0f ) { 
    	float t = 2.0f - x;
    	return t * t * t / 6.0f;
    }
    return 0.0f;
}


MitchellFilter::MitchellFilter() : Filter(2.0f) { setParameters(1.0f/3.0f, 1.0f/3.0f); }

float MitchellFilter::evaluate(float x) const
{
	x = fabs(x);
	if( x < 1.0f ) return p0 + x * x * (p2 + x * p3);
	if( x < 2.0f ) return q0 + x * (q1 + x * (q2 + x * q3));
	return 0.0f;
}

void MitchellFilter::setParameters(float b, float c)
{
	p0 = (6.0f -  2.0f * b) / 6.0f;
	p2 = (-18.0f + 12.0f * b + 6.0f * c) / 6.0f;
	p3 = (12.0f - 9.0f * b - 6.0f * c) / 6.0f;
	q0 = (8.0f * b + 24.0f * c) / 6.0f;
	q1 = (-12.0f * b - 48.0f * c) / 6.0f;
	q2 = (6.0f * b + 30.0f * c) / 6.0f;
	q3 = (-b - 6.0f * c) / 6.0f;
}


LanczosFilter::LanczosFilter() : Filter(3.0f) {}

float LanczosFilter::evaluate(float x) const
{
	x = fabs(x);
	if( x < 3.0f ) return sincf(PI * x) * sincf(PI * x / 3.0f);
	return 0.0f;
}


SincFilter::SincFilter(float w) : Filter(w) {}

float SincFilter::evaluate(float x) const
{
	return sincf(PI * x);
}


KaiserFilter::KaiserFilter(float w) : Filter(w) { setParameters(4.0f, 1.0f); }

float KaiserFilter::evaluate(float x) const
{
	const float sinc_value = sincf(PI * x * stretch);
	const float t = x / m_width;
	if ((1 - t * t) >= 0) return sinc_value * bessel0(alpha * sqrtf(1 - t * t)) / bessel0(alpha);
	else return 0;
}

void KaiserFilter::setParameters(float alpha, float stretch)
{
	this->alpha = alpha;
	this->stretch = stretch;
}



/// Ctor.
Kernel1::Kernel1(const Filter & f, int iscale, int samples/*= 32*/)
{
	nvDebugCheck(iscale > 1);
	nvDebugCheck(samples > 0);
	
	const float scale = 1.0f / iscale;
	
	m_width = f.width() * iscale;
	m_windowSize = (int)ceilf(2 * m_width);
	m_data = new float[m_windowSize];
	
	const float offset = float(m_windowSize) / 2;
	
	float total = 0.0f;
	for (int i = 0; i < m_windowSize; i++)
	{
		const float sample = f.sampleBox(i - offset, scale, samples);
		m_data[i] = sample;
		total += sample;
	}
	
	const float inv = 1.0f / total;
	for (int i = 0; i < m_windowSize; i++)
	{
		m_data[i] *= inv;
	}
}

/// Dtor.
Kernel1::~Kernel1()
{
	delete m_data;
}

/// Print the kernel for debugging purposes.
void Kernel1::debugPrint()
{
	for (int i = 0; i < m_windowSize; i++) {
		nvDebug("%d: %f\n", i, m_data[i]);
	}
}



/// Ctor.
Kernel2::Kernel2(uint ws) : m_windowSize(ws)
{
	m_data = new float[m_windowSize * m_windowSize];
}

/// Copy ctor.
Kernel2::Kernel2(const Kernel2 & k) : m_windowSize(k.m_windowSize)
{
	m_data = new float[m_windowSize * m_windowSize];
	for (uint i = 0; i < m_windowSize * m_windowSize; i++) {
		m_data[i] = k.m_data[i];
	}
}


/// Dtor.
Kernel2::~Kernel2()
{
	delete m_data;
}

/// Normalize the filter.
void Kernel2::normalize()
{
	float total = 0.0f;
	for(uint i = 0; i < m_windowSize*m_windowSize; i++) {
		total += fabs(m_data[i]);
	}
	
	float inv = 1.0f / total;
	for(uint i = 0; i < m_windowSize*m_windowSize; i++) {
		m_data[i] *= inv;
	}
}

/// Transpose the kernel.
void Kernel2::transpose()
{
	for(uint i = 0; i < m_windowSize; i++) {
		for(uint j = i+1; j < m_windowSize; j++) {
			swap(m_data[i*m_windowSize + j], m_data[j*m_windowSize + i]);
		}
	}
}

/// Init laplacian filter, usually used for sharpening.
void Kernel2::initLaplacian()
{
	nvDebugCheck(m_windowSize == 3);
//	m_data[0] = -1; m_data[1] = -1; m_data[2] = -1;
//	m_data[3] = -1; m_data[4] = +8; m_data[5] = -1;
//	m_data[6] = -1; m_data[7] = -1; m_data[8] = -1;	
	
	m_data[0] = +0; m_data[1] = -1; m_data[2] = +0;
	m_data[3] = -1; m_data[4] = +4; m_data[5] = -1;
	m_data[6] = +0; m_data[7] = -1; m_data[8] = +0;	
	
//	m_data[0] = +1; m_data[1] = -2; m_data[2] = +1;
//	m_data[3] = -2; m_data[4] = +4; m_data[5] = -2;
//	m_data[6] = +1; m_data[7] = -2; m_data[8] = +1;	
}


/// Init simple edge detection filter.
void Kernel2::initEdgeDetection()
{
	nvCheck(m_windowSize == 3);
	m_data[0] = 0; m_data[1] = 0; m_data[2] = 0;
	m_data[3] =-1; m_data[4] = 0; m_data[5] = 1;
	m_data[6] = 0; m_data[7] = 0; m_data[8] = 0;
}

/// Init sobel filter.
void Kernel2::initSobel()
{
	if (m_windowSize == 3)
	{
		m_data[0] = -1; m_data[1] = 0; m_data[2] = 1;
		m_data[3] = -2; m_data[4] = 0; m_data[5] = 2;
		m_data[6] = -1; m_data[7] = 0; m_data[8] = 1;
	}
	else if (m_windowSize == 5)
	{
		float elements[] = {
            -1, -2, 0, 2, 1,
            -2, -3, 0, 3, 2,
            -3, -4, 0, 4, 3,
            -2, -3, 0, 3, 2,
            -1, -2, 0, 2, 1
		};

		for (int i = 0; i < 5*5; i++) {
			m_data[i] = elements[i];
		}
	}
	else if (m_windowSize == 7)
	{
		float elements[] = {
            -1, -2, -3, 0, 3, 2, 1,
            -2, -3, -4, 0, 4, 3, 2,
            -3, -4, -5, 0, 5, 4, 3,
            -4, -5, -6, 0, 6, 5, 4,
            -3, -4, -5, 0, 5, 4, 3,
            -2, -3, -4, 0, 4, 3, 2,
            -1, -2, -3, 0, 3, 2, 1
		};

		for (int i = 0; i < 7*7; i++) {
			m_data[i] = elements[i];
		}
	}
	else if (m_windowSize == 9)
	{
		float elements[] = {
            -1, -2, -3, -4, 0, 4, 3, 2, 1,
            -2, -3, -4, -5, 0, 5, 4, 3, 2,
            -3, -4, -5, -6, 0, 6, 5, 4, 3,
            -4, -5, -6, -7, 0, 7, 6, 5, 4,
            -5, -6, -7, -8, 0, 8, 7, 6, 5,
            -4, -5, -6, -7, 0, 7, 6, 5, 4,
            -3, -4, -5, -6, 0, 6, 5, 4, 3,
            -2, -3, -4, -5, 0, 5, 4, 3, 2,
            -1, -2, -3, -4, 0, 4, 3, 2, 1
		};
		
		for (int i = 0; i < 9*9; i++) {
			m_data[i] = elements[i];
		}
	}
}

/// Init prewitt filter.
void Kernel2::initPrewitt()
{
	if (m_windowSize == 3)
	{
		m_data[0] = -1; m_data[1] = 0; m_data[2] = -1;
		m_data[3] = -1; m_data[4] = 0; m_data[5] = -1;
		m_data[6] = -1; m_data[7] = 0; m_data[8] = -1;
	}
	else if (m_windowSize == 5)
	{
		// @@ Is this correct?
		float elements[] = {
            -2, -1, 0, 1, 2,
            -2, -1, 0, 1, 2,
            -2, -1, 0, 1, 2,
            -2, -1, 0, 1, 2,
            -2, -1, 0, 1, 2
		};

		for (int i = 0; i < 5*5; i++) {
			m_data[i] = elements[i];
		}
	}
}

/// Init blended sobel filter.
void Kernel2::initBlendedSobel(const Vector4 & scale)
{
	nvCheck(m_windowSize == 9);

	{
		const float elements[] = {
            -1, -2, -3, -4, 0, 4, 3, 2, 1,
            -2, -3, -4, -5, 0, 5, 4, 3, 2,
            -3, -4, -5, -6, 0, 6, 5, 4, 3,
            -4, -5, -6, -7, 0, 7, 6, 5, 4,
            -5, -6, -7, -8, 0, 8, 7, 6, 5,
            -4, -5, -6, -7, 0, 7, 6, 5, 4,
            -3, -4, -5, -6, 0, 6, 5, 4, 3,
            -2, -3, -4, -5, 0, 5, 4, 3, 2,
            -1, -2, -3, -4, 0, 4, 3, 2, 1
		};
		
		for (int i = 0; i < 9*9; i++) {
			m_data[i] = elements[i] * scale.w();
		}
	}
	{
		const float elements[] = {
            -1, -2, -3, 0, 3, 2, 1,
            -2, -3, -4, 0, 4, 3, 2,
            -3, -4, -5, 0, 5, 4, 3,
            -4, -5, -6, 0, 6, 5, 4,
            -3, -4, -5, 0, 5, 4, 3,
            -2, -3, -4, 0, 4, 3, 2,
            -1, -2, -3, 0, 3, 2, 1,
		};

		for (int i = 0; i < 7; i++) {
			for (int e = 0; e < 7; e++) {
				m_data[(i + 1) * 9 + e + 1] += elements[i * 7 + e] * scale.z();
			}
		}
	}
	{
		const float elements[] = {
            -1, -2, 0, 2, 1,
            -2, -3, 0, 3, 2,
            -3, -4, 0, 4, 3,
            -2, -3, 0, 3, 2,
            -1, -2, 0, 2, 1
		};

		for (int i = 0; i < 5; i++) {
			for (int e = 0; e < 5; e++) {
				m_data[(i + 2) * 9 + e + 2] += elements[i * 5 + e] * scale.y();
			}
		}
	}
	{
		const float elements[] = {
            -1, 0, 1,
            -2, 0, 2,
            -1, 0, 1,
		};

		for (int i = 0; i < 3; i++) {
			for (int e = 0; e < 3; e++) {
				m_data[(i + 3) * 9 + e + 3] += elements[i * 3 + e] * scale.x();
			}
		}
	}
}


PolyphaseKernel::PolyphaseKernel(const Filter & f, uint srcLength, uint dstLength, int samples/*= 32*/)
{
	nvDebugCheck(samples > 0);

	float scale = float(dstLength) / float(srcLength);
	const float iscale = 1.0f / scale;

	if (scale > 1) {
		// Upsampling.
		samples = 1;
		scale = 1;
	}

	m_length = dstLength;
	m_width = f.width() * iscale;
	m_windowSize = (int)ceilf(m_width * 2) + 1;

	m_data = new float[m_windowSize * m_length];
	memset(m_data, 0, sizeof(float) * m_windowSize * m_length);

	for (uint i = 0; i < m_length; i++)
	{
		const float center = (0.5f + i) * iscale;
		
		const int left = (int)floorf(center - m_width);
		const int right = (int)ceilf(center + m_width);
		nvDebugCheck(right - left <= m_windowSize);
		
		float total = 0.0f;
		for (int j = 0; j < m_windowSize; j++)
		{
			const float sample = f.sampleBox(left + j - center, scale, samples);
			
			m_data[i * m_windowSize + j] = sample;
			total += sample;
		}
		
		// normalize weights.
		for (int j = 0; j < m_windowSize; j++)
		{
			m_data[i * m_windowSize + j] /= total;
		}
	}
}

PolyphaseKernel::~PolyphaseKernel()
{
	delete [] m_data;
}


/// Print the kernel for debugging purposes.
void PolyphaseKernel::debugPrint() const
{
	for (uint i = 0; i < m_length; i++)
	{
		nvDebug("%d: ", i);
		for (int j = 0; j < m_windowSize; j++)
		{
			nvDebug(" %6.4f", m_data[i * m_windowSize + j]);
		}
		nvDebug("\n");
	}
}

