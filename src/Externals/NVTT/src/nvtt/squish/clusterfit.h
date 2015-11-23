/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the 
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to 
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */
   
#ifndef SQUISH_CLUSTERFIT_H
#define SQUISH_CLUSTERFIT_H

#include "squish.h"
#include "maths.h"
#include "simd.h"
#include "colourfit.h"

namespace squish {

class ClusterFit : public ColourFit
{
public:
	ClusterFit();

	void SetColourSet( ColourSet const* colours, int flags );

	void SetMetric(float r, float g, float b);
	float GetBestError() const;

private:
	virtual void Compress3( void* block );
	virtual void Compress4( void* block );

	void Reorder( Vec3::Arg principle );

	Vec3 m_principle;
#if SQUISH_USE_SIMD
	Vec4 SolveLeastSquares( Vec4& start, Vec4& end ) const;

	Vec4 m_weighted[16];
	Vec4 m_unweighted[16];
	Vec4 m_weights[16];
	Vec4 m_metric;
	Vec4 m_metricSqr;
	Vec4 m_alpha[16];
	Vec4 m_beta[16];
	Vec4 m_xxsum;
	Vec4 m_besterror;
#else
	float SolveLeastSquares( Vec3& start, Vec3& end ) const;

	Vec3 m_weighted[16];
	Vec3 m_unweighted[16];
	float m_weights[16];
	Vec3 m_metric;
	Vec3 m_metricSqr;
	float m_alpha[16];
	float m_beta[16];
	Vec3 m_xxsum;
	float m_besterror;
#endif
	int m_order[16];
};

} // namespace squish

#endif // ndef SQUISH_CLUSTERFIT_H
