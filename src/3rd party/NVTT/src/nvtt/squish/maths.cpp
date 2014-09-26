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
   
#include "maths.h"
#include "simd.h"
#include <cfloat>

namespace squish {

Sym3x3 ComputeWeightedCovariance( int n, Vec3 const* points, float const* weights, Vec3::Arg metric )
{
	// compute the centroid
	float total = 0.0f;
	Vec3 centroid( 0.0f );
	for( int i = 0; i < n; ++i )
	{
		total += weights[i];
		centroid += weights[i]*points[i];
	}
	centroid /= total;

	// accumulate the covariance matrix
	Sym3x3 covariance( 0.0f );
	for( int i = 0; i < n; ++i )
	{
		Vec3 a = (points[i] - centroid) * metric;
		Vec3 b = weights[i]*a;
		
		covariance[0] += a.X()*b.X();
		covariance[1] += a.X()*b.Y();
		covariance[2] += a.X()*b.Z();
		covariance[3] += a.Y()*b.Y();
		covariance[4] += a.Y()*b.Z();
		covariance[5] += a.Z()*b.Z();
	}
	
	// return it
	return covariance;
}


static Vec3 EstimatePrincipleComponent( Sym3x3 const& matrix )
{
	Vec3 const row0(matrix[0], matrix[1], matrix[2]);
	Vec3 const row1(matrix[1], matrix[3], matrix[4]);
	Vec3 const row2(matrix[2], matrix[4], matrix[5]);

	float r0 = Dot(row0, row0);
	float r1 = Dot(row1, row1);
	float r2 = Dot(row2, row2);

	if (r0 > r1 && r0 > r2) return row0;
	if (r1 > r2) return row1;
	return row2;
}


#define POWER_ITERATION_COUNT   8

#if SQUISH_USE_SIMD

Vec3 ComputePrincipleComponent( Sym3x3 const& matrix )
{
	Vec4 const row0( matrix[0], matrix[1], matrix[2], 0.0f );
	Vec4 const row1( matrix[1], matrix[3], matrix[4], 0.0f );
	Vec4 const row2( matrix[2], matrix[4], matrix[5], 0.0f );

	//Vec4 v = VEC4_CONST( 1.0f );
	//Vec4 v = row0; // row1, row2

	Vec3 v3 = EstimatePrincipleComponent( matrix );
	Vec4 v( v3.X(), v3.Y(), v3.Z(), 0.0f );

	for( int i = 0; i < POWER_ITERATION_COUNT; ++i )
	{
		// matrix multiply
		Vec4 w = row0*v.SplatX();
		w = MultiplyAdd(row1, v.SplatY(), w);
		w = MultiplyAdd(row2, v.SplatZ(), w);

		// get max component from xyz in all channels
		Vec4 a = Max(w.SplatX(), Max(w.SplatY(), w.SplatZ()));

		// divide through and advance
		v = w*Reciprocal(a);
	}
	return v.GetVec3();
}

#else

Vec3 ComputePrincipleComponent( Sym3x3 const& matrix )
{
	Vec3 v = EstimatePrincipleComponent( matrix );
	for (int i = 0; i < POWER_ITERATION_COUNT; i++)
	{
		float x = v.X() * matrix[0] + v.Y() * matrix[1] + v.Z() * matrix[2];
		float y = v.X() * matrix[1] + v.Y() * matrix[3] + v.Z() * matrix[4];
		float z = v.X() * matrix[2] + v.Y() * matrix[4] + v.Z() * matrix[5];
		
		float norm = std::max(std::max(x, y), z);
		float iv = 1.0f / norm;
		if (norm == 0.0f) {		// @@ I think this is not necessary in this case!!
			return Vec3(0.0f);
		}
		
		v = Vec3(x*iv, y*iv, z*iv);
	}

	return v;
}

#endif

} // namespace squish
