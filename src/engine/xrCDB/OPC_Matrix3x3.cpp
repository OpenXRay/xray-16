///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for 3x3 matrices.
 *	\file		IceMatrix3x3.cpp
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	3x3 matrix.
 *	DirectX-compliant, ie row-column order, ie m[Row][Col].
 *	Same as:
 *	m11  m12  m13  first row.
 *	m21  m22  m23  second row.
 *	m31  m32  m33  third row.
 *	Stored in memory as m11 m12 m13 m21...
 *
 *	Multiplication rules:
 *
 *	[x'y'z'] = [xyz][M]
 *
 *	x' = x*m11 + y*m21 + z*m31
 *	y' = x*m12 + y*m22 + z*m32
 *	z' = x*m13 + y*m23 + z*m33
 *
 *	\class		Matrix3x3
 *	\author		Pierre Terdiman
 *	\version	1.0
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "stdafx.h"
#pragma hdrstop

using namespace IceMaths;

// Cast operator
Matrix3x3::operator Matrix4x4() const
{
	return Matrix4x4(
	m[0][0],	m[0][1],	m[0][2],	0.0f,
	m[1][0],	m[1][1],	m[1][2],	0.0f,
	m[2][0],	m[2][1],	m[2][2],	0.0f,
	0.0f,		0.0f,		0.0f,		1.0f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Creates a rotation matrix that rotates a vector3 "from" into another vector3 "to".
 *	Original code by Tomas Möller. It has been modified to match ICE maths conventions (vector3 * matrix)
 *	\param		from	[in] normalized source vector3
 *	\param		to		[in] normalized destination vector3
 *	\return		Self-Reference
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Matrix3x3& Matrix3x3::FromTo(const Point& from, const Point& to)
{
	#define EPSILON 0.000001f

	Point v = from^to;
	float e = from|to;

	// "from" almost or equal to "to"-vector3?
	if(e>1.0f - EPSILON)
	{
		// return identity
		Identity();
	}
	// "from" almost or equal to negated "to"?
	else if(e<-1.0f + EPSILON)
	{
		// Left= from ^ (1,0,0)
		Point Left(0.0f, from.z, -from.y);
		// Was left=CROSS(from,(1,0,0)) a good choice?
		if((Left|Left)<EPSILON)
		{
			// Here we now that left = CROSS(from, (1,0,0)) will be a good choice
			Left = Point(-from.z, 0.0f, from.x);
		}
		// Normalize "left"
		Left.Normalize();

		Point Up = Left^from;
		/* now we have a coordinate system, i.e., a basis;    */
		/* M=(from, up, left), and we want to rotate to:      */
		/* N=(-from, up, -left). This is done with the matrix:*/
		/* N*M^T where M^T is the transpose of M              */
		float fxx=-from.x*from.x; float fyy=-from.y*from.y; float fzz=-from.z*from.z;
		float fxy=-from.x*from.y; float fxz=-from.x*from.z; float fyz=-from.y*from.z;

		float uxx=Up.x*Up.x; float uyy=Up.y*Up.y; float uzz=Up.z*Up.z;
		float uxy=Up.x*Up.y; float uxz=Up.x*Up.z; float uyz=Up.y*Up.z;

		float lxx=-Left.x*Left.x; float lyy=-Left.y*Left.y; float lzz=-Left.z*Left.z;
		float lxy=-Left.x*Left.y; float lxz=-Left.x*Left.z; float lyz=-Left.y*Left.z;
		// symmetric matrix
		m[0][0]=fxx+uxx+lxx;	m[1][0]=fxy+uxy+lxy;	m[2][0]=fxz+uxz+lxz;
		m[0][1]=m[1][0];		m[1][1]=fyy+uyy+lyy;	m[2][1]=fyz+uyz+lyz;
		m[0][2]=m[2][0];		m[1][2]=m[2][1];		m[2][2]=fzz+uzz+lzz;
	}
	else	// the most common case, unless "from"="to", or "from"=-"to"
	{
	#if 0
		// unoptimized version - a good compiler will optimize this.
		h=(1.0-e)/DOT(v,v);
		mtx[0][0]=e+h*v[0]*v[0];    mtx[0][1]=h*v[0]*v[1]-v[2]; mtx[0][2]=h*v[0]*v[2]+v[1];
		mtx[1][0]=h*v[0]*v[1]+v[2]; mtx[1][1]=e+h*v[1]*v[1];    mtx[1][2]=h*v[1]*v[2]-v[0];
		mtx[2][0]=h*v[0]*v[2]-v[1]; mtx[2][1]=h*v[1]*v[2]+v[0]; mtx[2][2]=e+h*v[2]*v[2];
	#else
		// ...otherwise use this hand optimized version (9 mults less)
		float h=(1.0f - e) / (v|v);
		float hvx  = h*v.x;
		float hvz  = h*v.z;
		float hvxy = hvx*v.y;
		float hvxz = hvx*v.z;
		float hvyz = hvz*v.y;
		m[0][0]=e+hvx*v.x;	m[1][0]=hvxy-v.z;		m[2][0]=hvxz+v.y;
		m[0][1]=hvxy+v.z;	m[1][1]=e+h*v.y*v.y;	m[2][1]=hvyz-v.x;
		m[0][2]=hvxz-v.y;	m[1][2]=hvyz+v.x;		m[2][2]=e+hvz*v.z;
	#endif
	}
	return *this;
}

