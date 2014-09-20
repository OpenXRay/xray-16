///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for 4x4 matrices.
 *	\file		IceMatrix4x4.cpp
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	4x4 matrix.
 *	DirectX-compliant, ie row-column order, ie m[Row][Col].
 *	Same as:
 *	m11  m12  m13  m14	first row.
 *	m21  m22  m23  m24	second row.
 *	m31  m32  m33  m34	third row.
 *	m41  m42  m43  m44	fourth row.
 *	Translation is (m41, m42, m43), (m14, m24, m34, m44) = (0, 0, 0, 1).
 *	Stored in memory as m11 m12 m13 m14 m21...
 *
 *	Multiplication rules:
 *
 *	[x'y'z'1] = [xyz1][M]
 *
 *	x' = x*m11 + y*m21 + z*m31 + m41
 *	y' = x*m12 + y*m22 + z*m32 + m42
 *	z' = x*m13 + y*m23 + z*m33 + m43
 *	1' =     0 +     0 +     0 + m44
 *
 *	\class		Matrix4x4
 *	\author		Pierre Terdiman
 *	\version	1.0
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "stdafx.h"
#pragma hdrstop

using namespace IceMaths;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	A global function to invert a PR matrix. (which only contains a rotation and a translation)
 *	\relates	Matrix4x4
 *	\fn			InvertPRMatrix(Matrix4x4& dest, const Matrix4x4& src)
 *	\param		dest			[out] destination matrix
 *	\param		src				[in] source matrix
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ICEMATHS_API void IceMaths::InvertPRMatrix(Matrix4x4& dest, const Matrix4x4& src)
{
	dest.m[0][0] = src.m[0][0];
	dest.m[1][0] = src.m[0][1];
	dest.m[2][0] = src.m[0][2];
	dest.m[3][0] = -(src.m[3][0]*src.m[0][0] + src.m[3][1]*src.m[0][1] + src.m[3][2]*src.m[0][2]);

	dest.m[0][1] = src.m[1][0];
	dest.m[1][1] = src.m[1][1];
	dest.m[2][1] = src.m[1][2];
	dest.m[3][1] = -(src.m[3][0]*src.m[1][0] + src.m[3][1]*src.m[1][1] + src.m[3][2]*src.m[1][2]);

	dest.m[0][2] = src.m[2][0];
	dest.m[1][2] = src.m[2][1];
	dest.m[2][2] = src.m[2][2];
	dest.m[3][2] = -(src.m[3][0]*src.m[2][0] + src.m[3][1]*src.m[2][1] + src.m[3][2]*src.m[2][2]);

	dest.m[0][3] = 0.0f;
	dest.m[1][3] = 0.0f;
	dest.m[2][3] = 0.0f;
	dest.m[3][3] = 1.0f;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Compute the cofactor of the Matrix at a specified location
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float Matrix4x4::CoFactor(udword row, udword col) const
{
	return	 (( m[(row+1)&3][(col+1)&3]*m[(row+2)&3][(col+2)&3]*m[(row+3)&3][(col+3)&3] +
				m[(row+1)&3][(col+2)&3]*m[(row+2)&3][(col+3)&3]*m[(row+3)&3][(col+1)&3] +
				m[(row+1)&3][(col+3)&3]*m[(row+2)&3][(col+1)&3]*m[(row+3)&3][(col+2)&3])
			-  (m[(row+3)&3][(col+1)&3]*m[(row+2)&3][(col+2)&3]*m[(row+1)&3][(col+3)&3] +
				m[(row+3)&3][(col+2)&3]*m[(row+2)&3][(col+3)&3]*m[(row+1)&3][(col+1)&3] +
				m[(row+3)&3][(col+3)&3]*m[(row+2)&3][(col+1)&3]*m[(row+1)&3][(col+2)&3])) * ((row + col) & 1 ? -1.0f : +1.0f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Compute the determinant of the Matrix
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float Matrix4x4::Determinant() const
{
	return	m[0][0] * CoFactor(0, 0) +
			m[0][1] * CoFactor(0, 1) +
			m[0][2] * CoFactor(0, 2) +
			m[0][3] * CoFactor(0, 3);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Compute the inverse of the matrix
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Matrix4x4& Matrix4x4::Invert()
{
	float Det = Determinant();
	Matrix4x4 Temp;

	if(_abs(Det) < MATRIX4X4_EPSILON)
		return	*this;		// The matrix is not invertible! Singular case!

	float IDet = 1.0f / Det;

	Temp.m[0][0] = CoFactor(0,0) * IDet;
	Temp.m[1][0] = CoFactor(0,1) * IDet;
	Temp.m[2][0] = CoFactor(0,2) * IDet;
	Temp.m[3][0] = CoFactor(0,3) * IDet;
	Temp.m[0][1] = CoFactor(1,0) * IDet;
	Temp.m[1][1] = CoFactor(1,1) * IDet;
	Temp.m[2][1] = CoFactor(1,2) * IDet;
	Temp.m[3][1] = CoFactor(1,3) * IDet;
	Temp.m[0][2] = CoFactor(2,0) * IDet;
	Temp.m[1][2] = CoFactor(2,1) * IDet;
	Temp.m[2][2] = CoFactor(2,2) * IDet;
	Temp.m[3][2] = CoFactor(2,3) * IDet;
	Temp.m[0][3] = CoFactor(3,0) * IDet;
	Temp.m[1][3] = CoFactor(3,1) * IDet;
	Temp.m[2][3] = CoFactor(3,2) * IDet;
	Temp.m[3][3] = CoFactor(3,3) * IDet;

	*this = Temp;

	return	*this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Compute a shadow matrix, used to draw planar stencil shadows.
 *	\param		light		[in] light source position.
 *	\param		p0			[in] first point on the projection plane.
 *	\param		p1			[in] second point on the projection plane.
 *	\param		p2			[in] third point on the projection plane.
 *	\return		Self-Reference
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Matrix4x4& Matrix4x4::Shadow(const Point& light, const Point& p0, const Point& p1, const Point& p2)
{
	// The 3 input vertices form the projection plane.

	// Compute the plane equation
	Point n = ((p0-p1)^(p1-p2)).Normalize();
	float D		= -(p0|n);
	Plane PlaneEquation;
	float Coeff;
	if(_abs(D)<0.0001f)	Coeff = -1.0f;
	else					Coeff = -1.0f / _abs(D);
	PlaneEquation.n.x = n.x * Coeff;
	PlaneEquation.n.y = n.y * Coeff;
	PlaneEquation.n.z = n.z * Coeff;
	PlaneEquation.d   = D * Coeff;

	// Plane equation must be normalized!
	float dot = PlaneEquation.n.x*light.x + PlaneEquation.n.y*light.y + PlaneEquation.n.z*light.z + PlaneEquation.d;

	m[0][0] = dot - light.x*PlaneEquation.n.x;
	m[1][0] =     - light.x*PlaneEquation.n.y;
	m[2][0] =     - light.x*PlaneEquation.n.z;
	m[3][0] =     - light.x*PlaneEquation.d;

	m[0][1] =     - light.y*PlaneEquation.n.x;
	m[1][1] = dot - light.y*PlaneEquation.n.y;
	m[2][1] =     - light.y*PlaneEquation.n.z;
	m[3][1] =     - light.y*PlaneEquation.d;

	m[0][2] =     - light.z*PlaneEquation.n.x;
	m[1][2] =     - light.z*PlaneEquation.n.y;
	m[2][2] = dot - light.z*PlaneEquation.n.z;
	m[3][2] =     - light.z*PlaneEquation.d;

	m[0][3] =     - PlaneEquation.n.x;
	m[1][3] =     - PlaneEquation.n.y;
	m[2][3] =     - PlaneEquation.n.z;
	m[3][3] = dot - PlaneEquation.d;

	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Compute a sphere map matrix.
 *	\param		scale		[in] scale factor.
 *	\return		Self-Reference
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Matrix4x4& Matrix4x4::SphereMap(float scale)
{
	Identity();
	m[0][0] = scale;
	m[1][1] = scale;
	m[3][0] = 0.5f;
	m[3][1] = 0.5f;
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Compute a self-shadowing matrix.
 *	\param		light		[in] light source position.
 *	\return		Self-Reference
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Matrix4x4& Matrix4x4::SelfShadow(const Point& light)
{
	Point Light = light;
	Light.Normalize();

	Zero();
	m[0][0] = Light.x * 0.5f;
	m[0][1] = Light.y * 0.5f;
	m[0][2] = Light.z * 0.5f;
	m[0][3] = 0.5f;
	m[3][3] = 1.0f;
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Compute a rotozoom matrix.
 *	\param		angle		[in] rotation angle
 *	\param		zoom		[in] zoom factor
 *	\param		posx		[in] x translation
 *	\param		posy		[in] y translation
 *	\return		Self-Reference
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Matrix4x4& Matrix4x4::Rotozoom(float angle, float zoom, float posx, float posy)
{
	RotZ(angle);
	Scale(zoom, zoom, zoom);
	SetTrans(posx, posy, 0.0f);
	return *this;
}

// ### must be optimized... consider using the 3x3 version
Matrix4x4& Matrix4x4::Rot(float angle, Point& p1, Point& p2)
{
	Point Axis = (p2 - p1).Normalize();

	Matrix4x4 T, InvT;
	T.Identity();
	T.SetTrans(-p1.x, -p1.y, -p1.z);
	InvT = T;
	InvT.Invert();

	Matrix4x4 Rx, InvRx;
	Rx.Identity();
	float d = _sqrt(Axis.y*Axis.y + Axis.z*Axis.z);
	if(d!=0.0f)
	{
		float CosAngle = Axis.z / d;
		float SinAngle = Axis.y / d;
		Rx.SetRow(1, Point(0.0f, CosAngle, SinAngle));
		Rx.SetRow(2, Point(0.0f, -SinAngle, CosAngle));
	}
	InvRx = Rx;
	InvRx.Invert();

	Matrix4x4 Ry, InvRy;
	Ry.Identity();
	Ry.SetRow(0, Point(d,        0.0f,  Axis.x));
	Ry.SetRow(2, Point(-Axis.x,  0.0f,  d));
	InvRy = Ry;
	InvRy.Invert();

	Matrix4x4 Rz;
	Rz.RotZ(angle);

	Matrix4x4 Combo = T * Rx * Ry * Rz * InvRy * InvRx * InvT;
	*this = Combo;
	return *this;
}


#ifdef OLDIES

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LU Backward substitution
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input	: 
//	- indx, 
//	- b, 
// Output	: None
// Return	: None
// Exception: None
// Remark	: None

void	Matrix::LUBackwardSubstitution( sdword *indx, float *b )
{
	sdword	i, j, ii=-1, ip;
	float	sum;

	for ( i=0; i<4; i++ )
	{
		ip = indx[i];
		sum = b[ip];
		b[ip] = b[i];

		if (ii>=0)
		{
			for (j=ii; j<=i-1; j++)
				sum -= (*this)(i, j) * b[j];
		}
		else if (sum != 0.0f)
			ii = i;
		b[i] = sum;
	}

	for ( i=3; i>=0; i-- )
	{
		sum = b[i];
		for (j=i+1; j<4; j++)
			sum -= (*this)(i, j) * b[j];
		b[i] = sum / (*this)(i, i);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LU decomposition
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input	: 
//	- indx, 
//	- d, 
// Output	: None
// Return	: None
// Exception: None
// Remark	: None

void	Matrix::LUDecomposition( sdword* indx, float* d )
{
	float	vv[4];               /* implicit scale for each row */
	float	big, dum, sum, tmp;
	sdword		i, imax, j, k;

	*d = 1.0f;
	for ( i=0; i<4; i++ )
	{
		big = 0.0f;
		for (j=0; j<4; j++)
			if ((tmp = (float) _abs( (*this)(i, j) )) > big)
				big = tmp;
		/*
		if (big == 0.0f) {
			printf("ludcmp(): singular matrix found...\n");
			exit(1);
		}
		*/
		vv[i] = 1.0f/big;
	}
	for ( j=0; j<4; j++ )
	{
		for ( i=0; i<j; i++ )
		{
			sum = (*this)(i, j);
			for (k=0; k<i; k++)
				sum -= (*this)(i, k) * (*this)(k, j);
			(*this)(i, j) = sum;
		}
		big = 0.0f;
		for ( i=j; i<4; i++ )
		{
			sum = (*this)(i, j);
			for (k=0; k<j; k++)
				sum -= (*this)(i, k) * (*this)(k, j);
			(*this)(i, j) = sum;
			if ((dum = vv[i] * (float) _abs(sum)) >= big)
			{
				big = dum;
				imax = i;
			}
		}
		if (j != imax)
		{
			for (k=0; k<4; k++)
			{
				dum = (*this)(imax, k);
				(*this)(imax, k) = (*this)(j, k);
				(*this)(j, k) = dum;
			}
			*d = -(*d);
			vv[imax] = vv[j];
		}
		indx[j] = imax;
		if ((*this)(j, j) == 0.0f)
			(*this)(j, j) = 1.0e-20f;      /* can be 0.0 also... */
		if (j != 3)
		{
			dum = 1.0f / (*this)(j, j);
			for (i=j+1; i<4; i++)
				(*this)(i, j) *= dum;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A method to compute a view matrix from an angle axis
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input	: axis, angle	the view parameters
// Output	: None
// Return	: None
// Exception: None
// Remark	: None

Matrix& Matrix::ComputeAxisMatrix(Point& axis, float angle)
{
	MakeIdentity();

	float length = axis.Magnitude();
	// Normalize the z basis vector3
	axis /= length;

	// Get the dot product, and calculate the projection of the z basis
	// vector3 onto the up vector3. The projection is the y basis vector3.
	float dotProduct = Point(0, 1, 0) | axis;
	Point Up = Point(0, 1, 0) - dotProduct * axis;

	// This is to prevent bogus view matrix (up view vector3 equals to axis)
	if (Up.Magnitude() < 1e-6f)	{
		Up = Point(0, 0, 1);
	}
	else	{
	// Normalize the y basis vector3
		Up /= length;
		Up.Normalize();
	}

	// The x basis vector3 is found simply with the cross product of the y
	// and z basis vectors
	Point Right = Up ^ axis;

	SetCol( 0, Right );
	SetCol( 1, Up );
	SetCol( 2, axis );
	Transpose();

	return *this;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operator to cast a matrix to a PRS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input	: None
// Output	: None
// Return	: None
// Exception: None
// Remark	: None

Matrix::operator	PRS() const
{
	PRS			Cast;

	udword		dwRow;
	Matrix3x3	Orthonormal;
	float		ScaleFactor;
	Point		Scale, Row, NormalizedRow;

	if ( IsIdentity() )
	{	// The special case of the identity matrix
		Cast.SetScale( 1.0f, 1.0f, 1.0f ).SetQuaternion( (Quat) (*this) );
	}
	else
	{
		for ( dwRow=0; dwRow<3; dwRow++ )
		{
			Row = *GetRow( dwRow );
			Scale[dwRow] = ScaleFactor = Row.Magnitude();

			if ( _abs(ScaleFactor) > mEpsilon )
				NormalizedRow = Row / ScaleFactor;
			else
			{
				NormalizedRow[0] = NormalizedRow[1] = NormalizedRow[2] = 0.0f;
				NormalizedRow[dwRow] = 1.0f;
			}
			Orthonormal.SetRow( dwRow, NormalizedRow );
		}

		// Build the final PRS
		Cast.SetQuaternion( (Quat) Orthonormal ).SetPosition( *GetRow( 3 ) ).SetScale( Scale );
	}

	return	Cast;
}

#endif