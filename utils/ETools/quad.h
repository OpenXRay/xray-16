/* Copyright (C) Tom Forsyth, 2001. 
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) Tom Forsyth, 2001"
 */
#ifndef quadH
#define quadH

// This Quad only records the error due to the vertex positions.
// A real implementation needs to use smarter QEMs that take account of
// other vertex attrbiutes such as normals, texture coords, vertex colours,
// etc. Hugues Hoppe's version of this looks like the most complete and simple
// version.
//  The algo also needs to do smarter things than just look at QEMs,
// such as prevent normal-flipping, deal with degenerate collapses,
// deal with non-manifold meshes, etc.
//
// But this will do for my purposes because:
// (a) it's simple.
// (b) it's flexible - all vertices have a position.
// (c) it is good enough to give plausable collapse orders.

struct Quad3
{
	float A00, A01, A02;
	float      A11, A12;
	float           A22;
	float B0, B1, B2;
	float C;


	Quad3 ( void )
	{
		A00 = A01 = A02 = A11 = A12 = A22 = 0.0f;
		B0 = B1 = B2 = 0.0f;
		C = 0.0f;
	}

	// Create a quad from a triangle (numbered clockwise).
	Quad3 ( const Fvector3 &vec1, const Fvector3 &vec2, const Fvector3 &vec3 )
	{
		Fvector3 vec12; vec12.sub(vec2,vec1);
		Fvector3 vec13; vec13.sub(vec3,vec1);

		Fvector3 vNorm;
		vNorm.crossproduct(vec12,vec13);
		float fArea = vNorm.magnitude();
		vNorm.div(vNorm,fArea);
		// Use the area of the tri, not the parallelogram.
		fArea *= 0.5f;

		// Find the distance of the origin from the plane, so that
		// P*N + D = 0
		// => D = -P*N
		float fDist = - vNorm.dotproduct(vec1);

		// And now form the Quadric.
		// A = NNt (and bin the lower half, since it is symmetrical).
		// B = D*N
		// C = D^2
		// The quadric is weighted by the area of the tri.
		A00 = fArea * vNorm.x * vNorm.x;
		A01 = fArea * vNorm.x * vNorm.y;
		A02 = fArea * vNorm.x * vNorm.z;
		A11 = fArea * vNorm.y * vNorm.y;
		A12 = fArea * vNorm.y * vNorm.z;
		A22 = fArea * vNorm.z * vNorm.z;
		B0  = fArea * vNorm.x * fDist;
		B1  = fArea * vNorm.y * fDist;
		B2  = fArea * vNorm.z * fDist;
		C   = fArea * fDist   * fDist;
	}

	const float FindError ( const Fvector3 &vec )
	{
		return (
			A00 * vec.x * vec.x +
			A01 * vec.x * vec.y * 2 +
			A02 * vec.x * vec.z * 2 +
			A11 * vec.y * vec.y +
			A12 * vec.y * vec.z * 2 +
			A22 * vec.z * vec.z +
			B0  * vec.x * 2 +
			B1  * vec.y * 2 +
			B2  * vec.z * 2 +
			C
				);
	}

	Quad3 operator+ ( const Quad3 &q )
	{
		Quad3 rq;
		rq.A00 = A00 + q.A00;
		rq.A01 = A01 + q.A01;
		rq.A02 = A02 + q.A02;
		rq.A11 = A11 + q.A11;
		rq.A12 = A12 + q.A12;
		rq.A22 = A22 + q.A22;
		rq.B0  = B0  + q.B0 ;
		rq.B1  = B1  + q.B1 ;
		rq.B2  = B2  + q.B2 ;
		rq.C   = C   + q.C  ;
		return rq;
	}

	Quad3 &operator+= ( const Quad3 &q )
	{
		A00 += q.A00;
		A01 += q.A01;
		A02 += q.A02;
		A11 += q.A11;
		A12 += q.A12;
		A22 += q.A22;
		B0  += q.B0 ;
		B1  += q.B1 ;
		B2  += q.B2 ;
		C   += q.C  ;
		return (*this);
	}
};

#endif // quadH
