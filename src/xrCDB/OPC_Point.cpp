///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for 3D vectors.
 *	\file		IcePoint.cpp
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	3D point.
 *
 *	The name is "Point" instead of "Vector" since a vector3 is N-dimensional, whereas a point is an implicit "vector3 of dimension 3".
 *	So the choice was between "Point" and "Vector3", the first one looked better (IMHO).
 *
 *	Some people, then, use a typedef to handle both points & vectors using the same class: typedef Point Vector3;
 *	This is bad since it opens the door to a lot of confusion while reading the code. I know it may sounds weird but check this out:
 *
 *	\code
 *		Point P0,P1 = some 3D points;
 *		Point Delta = P1 - P0;
 *	\endcode
 *
 *	This compiles fine, although you should have written:
 *
 *	\code
 *		Point P0,P1 = some 3D points;
 *		Vector3 Delta = P1 - P0;
 *	\endcode
 *
 *	Subtle things like this are not caught at compile-time, and when you find one in the code, you never know whether it's a mistake
 *	from the author or something you don't get.
 *
 *	One way to handle it at compile-time would be to use different classes for Point & Vector3, only overloading operator "-" for vectors.
 *	But then, you get a lot of redundant code in thoses classes, and basically it's really a lot of useless work.
 *
 *	Another way would be to use homogeneous points: w=1 for points, w=0 for vectors. That's why the HPoint class exists. Now, to store
 *	your model's vertices and in most cases, you really want to use Points to save ram.
 *
 *	\class		Point
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
// WARNING: not inlined
//Point::operator HPoint() const	{ return HPoint(x, y, z, 0.0f); }

Point& Point::Refract(const Point& eye, const Point& n, float refractindex, Point& refracted)
{
	//	Point EyePt = eye position
	//	Point p = current vertex
	//	Point n = vertex normal
	//	Point rv = refracted vector3
	//	Eye vector3 - doesn't need to be normalized
	Point Env;
	Env.x = eye.x - x;
	Env.y = eye.y - y;
	Env.z = eye.z - z;

	float NDotE = n|Env;
	float NDotN = n|n;
	NDotE /= refractindex;

	// Refracted vector3
	refracted = n*NDotE - Env*NDotN;

	return *this;
}

Point& Point::ProjectToPlane(const Plane& p)
{
	*this-= (p.d + (*this|p.n))*p.n;
	return *this;
}

//### could be optimized
Point& Point::Unfold(Plane& p, Point& a, Point& b)
{
	ASSERT(!"Obsolete code");
/*	Point v = *this;

	// Form the plane of the triangle
	Plane TriPlane(a, b, v);

	// Compute rotation angle
	float CosAngle = p.n|TriPlane.n;
	float Angle = acosf(CosAngle);
	if(FastFabs(Angle)<0.0001f)	return *this;	// Point is already on plane // ## project

	// Rotate around (a,b)
	// 1) Move to origin
	Point p0t(0.0f, 0.0f, 0.0f);
	Point p1t = b - a;
	Point p2t = v - a;

	// 2) Rotate so that rotation axis = Z axis
	Matrix3x3 M;
	Point Axis = (p1t - p0t).Normalize();
	float DpX = FastFabs(Axis.x);
	float DpY = FastFabs(Axis.y);
	float DpZ = FastFabs(Axis.z);
			if(FastFabs((Axis|Point(0.0f, 0.0f, 1.0f)) - 1.0f) < 0.0001f)	M.Identity();
	else	if(FastFabs((Axis|Point(0.0f, 0.0f, 1.0f)) + 1.0f) < 0.0001f)	{ M.Identity(); Angle = -Angle; }
	else																	M.MapToZ(Axis);

	Point p0r = M * p0t;
	Point p1r = M * p1t;
	Point p2r = M * p2t;

	// 3) Rotate around original angle
	Matrix3x3 RotZ;
	RotZ.RotZ(-Angle);
	p0r *= RotZ;
	p1r *= RotZ;
	p2r *= RotZ;

	// 4) Rotate back (since M is a rotation matrix we don't invert or transpose it, just swap mul conventions)
	p0t = p0r * M;
	p1t = p1r * M;
	p2t = p2r * M;

	// 5) Translate back
	p0t += a;
	p1t += a;
	p2t += a;

	// 6) Check _new_ plane
	Plane pipo(p0t, p1t, p2t);
	Angle = p.n|pipo.n;	// Must be 1

	*this = p2t;
*/
	return *this;
}

Point& Point::Mult(const Matrix3x3& mat, const Point& a)
{
	x = a.x * mat.m[0][0] + a.y * mat.m[0][1] + a.z * mat.m[0][2];
	y = a.x * mat.m[1][0] + a.y * mat.m[1][1] + a.z * mat.m[1][2];
	z = a.x * mat.m[2][0] + a.y * mat.m[2][1] + a.z * mat.m[2][2];
	return *this;
}

Point& Point::Mult2(const Matrix3x3& mat1, const Point& a1, const Matrix3x3& mat2, const Point& a2)
{
	x = a1.x * mat1.m[0][0] + a1.y * mat1.m[0][1] + a1.z * mat1.m[0][2] + a2.x * mat2.m[0][0] + a2.y * mat2.m[0][1] + a2.z * mat2.m[0][2];
	y = a1.x * mat1.m[1][0] + a1.y * mat1.m[1][1] + a1.z * mat1.m[1][2] + a2.x * mat2.m[1][0] + a2.y * mat2.m[1][1] + a2.z * mat2.m[1][2];
	z = a1.x * mat1.m[2][0] + a1.y * mat1.m[2][1] + a1.z * mat1.m[2][2] + a2.x * mat2.m[2][0] + a2.y * mat2.m[2][1] + a2.z * mat2.m[2][2];
	return *this;
}

Point& Point::Mac(const Matrix3x3& mat, const Point& a)
{
	x += a.x * mat.m[0][0] + a.y * mat.m[0][1] + a.z * mat.m[0][2];
	y += a.x * mat.m[1][0] + a.y * mat.m[1][1] + a.z * mat.m[1][2];
	z += a.x * mat.m[2][0] + a.y * mat.m[2][1] + a.z * mat.m[2][2];
	return *this;
}

Point& Point::TransMult(const Matrix3x3& mat, const Point& a)
{
	x = a.x * mat.m[0][0] + a.y * mat.m[1][0] + a.z * mat.m[2][0];
	y = a.x * mat.m[0][1] + a.y * mat.m[1][1] + a.z * mat.m[2][1];
	z = a.x * mat.m[0][2] + a.y * mat.m[1][2] + a.z * mat.m[2][2];
	return *this;
}

Point& Point::Transform(const Point& r, const Matrix3x3& rotpos, const Point& linpos)
{
	x = r.x * rotpos.m[0][0] + r.y * rotpos.m[0][1] + r.z * rotpos.m[0][2] + linpos.x;
	y = r.x * rotpos.m[1][0] + r.y * rotpos.m[1][1] + r.z * rotpos.m[1][2] + linpos.y;
	z = r.x * rotpos.m[2][0] + r.y * rotpos.m[2][1] + r.z * rotpos.m[2][2] + linpos.z;
	return *this;
}

Point& Point::InvTransform(const Point& r, const Matrix3x3& rotpos, const Point& linpos)
{
	float sx = r.x - linpos.x;
	float sy = r.y - linpos.y;
	float sz = r.z - linpos.z;
	x = sx * rotpos.m[0][0] + sy * rotpos.m[1][0] + sz * rotpos.m[2][0];
	y = sx * rotpos.m[0][1] + sy * rotpos.m[1][1] + sz * rotpos.m[2][1];
	z = sx * rotpos.m[0][2] + sy * rotpos.m[1][2] + sz * rotpos.m[2][2];
	return *this;
}

