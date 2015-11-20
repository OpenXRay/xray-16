/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
	Copyright (c) 2006 Ignacio Castano                      icastano@nvidia.com

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
   
#ifndef SQUISH_WEIGHTEDCLUSTERFIT_H
#define SQUISH_WEIGHTEDCLUSTERFIT_H

#include "squish.h"
#include "maths.h"
#include "simd.h"
#include "colourfit.h"

namespace squish {

class WeightedClusterFit : public ColourFit
{
public:
	WeightedClusterFit();

	void SetColourSet( ColourSet const* colours, int flags );
	
	void SetMetric(float r, float g, float b);
	float GetBestError() const;

	// Make them public
	virtual void Compress3( void* block );
	virtual void Compress4( void* block );
	
private:

	Vec3 m_principle;

#if SQUISH_USE_SIMD
	Vec4 m_weighted[16];
	Vec4 m_metric;
	Vec4 m_metricSqr;
	Vec4 m_xxsum;
	Vec4 m_xsum;
	Vec4 m_besterror;
#else
	Vec3 m_weighted[16];
	float m_weights[16];
	Vec3 m_metric;
	Vec3 m_metricSqr;
	Vec3 m_xxsum;
	Vec3 m_xsum;
	float m_wsum;
	float m_besterror;
#endif

	int m_order[16];
};

} // namespace squish

#endif // ndef SQUISH_WEIGHTEDCLUSTERFIT_H
