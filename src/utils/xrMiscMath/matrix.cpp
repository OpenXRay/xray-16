#include "pch.hpp"

#include "xrCore/_matrix.h"
#include "xrCore/_quaternion.h"
#include "xrCore/xrDebug_macros.h"
#include "xrCore/xrDebug.h"

#include <limits>

template <typename T>
_matrix<T>& _matrix<T>::rotation(const _quaternion<T>& Q)
{
	T xx = Q.x*Q.x;
	T yy = Q.y*Q.y;
	T zz = Q.z*Q.z;
	T xy = Q.x*Q.y;
	T xz = Q.x*Q.z;
	T yz = Q.y*Q.z;
	T wx = Q.w*Q.x;
	T wy = Q.w*Q.y;
	T wz = Q.w*Q.z;

	_11 = 1 - 2 * (yy + zz);
	_12 = 2 * (xy - wz);
	_13 = 2 * (xz + wy);
	_14 = 0;
	_21 = 2 * (xy + wz);
	_22 = 1 - 2 * (xx + zz);
	_23 = 2 * (yz - wx);
	_24 = 0;
	_31 = 2 * (xz - wy);
	_32 = 2 * (yz + wx);
	_33 = 1 - 2 * (xx + yy);
	_34 = 0;
	_41 = 0;
	_42 = 0;
	_43 = 0;
	_44 = 1;
	return *this;
}

template <typename T>
_matrix<T>& _matrix<T>::mk_xform(const _quaternion<T>& Q, const Tvector& V)
{
	T xx = Q.x*Q.x;
	T yy = Q.y*Q.y;
	T zz = Q.z*Q.z;
	T xy = Q.x*Q.y;
	T xz = Q.x*Q.z;
	T yz = Q.y*Q.z;
	T wx = Q.w*Q.x;
	T wy = Q.w*Q.y;
	T wz = Q.w*Q.z;

	_11 = 1 - 2 * (yy + zz);
	_12 = 2 * (xy - wz);
	_13 = 2 * (xz + wy);
	_14 = 0;
	_21 = 2 * (xy + wz);
	_22 = 1 - 2 * (xx + zz);
	_23 = 2 * (yz - wx);
	_24 = 0;
	_31 = 2 * (xz - wy);
	_32 = 2 * (yz + wx);
	_33 = 1 - 2 * (xx + yy);
	_34 = 0;
	_41 = V.x;
	_42 = V.y;
	_43 = V.z;
	_44 = 1;
	return *this;
}

template <typename T>
_matrix<T>& _matrix<T>::identity()
{
	_11 = 1;
	_12 = 0;
	_13 = 0;
	_14 = 0;
	_21 = 0;
	_22 = 1;
	_23 = 0;
	_24 = 0;
	_31 = 0;
	_32 = 0;
	_33 = 1;
	_34 = 0;
	_41 = 0;
	_42 = 0;
	_43 = 0;
	_44 = 1;
	return *this;
}

// Multiply RES = A[4x4]*B[4x4] (WITH projection)
template <typename T>
_matrix<T>& _matrix<T>::mul(const _matrix<T>& A, const _matrix<T>& B)
{
	VERIFY((this != &A) && (this != &B));
	m[0][0] = A.m[0][0] * B.m[0][0] + A.m[1][0] * B.m[0][1] + A.m[2][0] * B.m[0][2] + A.m[3][0] * B.m[0][3];
	m[0][1] = A.m[0][1] * B.m[0][0] + A.m[1][1] * B.m[0][1] + A.m[2][1] * B.m[0][2] + A.m[3][1] * B.m[0][3];
	m[0][2] = A.m[0][2] * B.m[0][0] + A.m[1][2] * B.m[0][1] + A.m[2][2] * B.m[0][2] + A.m[3][2] * B.m[0][3];
	m[0][3] = A.m[0][3] * B.m[0][0] + A.m[1][3] * B.m[0][1] + A.m[2][3] * B.m[0][2] + A.m[3][3] * B.m[0][3];

	m[1][0] = A.m[0][0] * B.m[1][0] + A.m[1][0] * B.m[1][1] + A.m[2][0] * B.m[1][2] + A.m[3][0] * B.m[1][3];
	m[1][1] = A.m[0][1] * B.m[1][0] + A.m[1][1] * B.m[1][1] + A.m[2][1] * B.m[1][2] + A.m[3][1] * B.m[1][3];
	m[1][2] = A.m[0][2] * B.m[1][0] + A.m[1][2] * B.m[1][1] + A.m[2][2] * B.m[1][2] + A.m[3][2] * B.m[1][3];
	m[1][3] = A.m[0][3] * B.m[1][0] + A.m[1][3] * B.m[1][1] + A.m[2][3] * B.m[1][2] + A.m[3][3] * B.m[1][3];

	m[2][0] = A.m[0][0] * B.m[2][0] + A.m[1][0] * B.m[2][1] + A.m[2][0] * B.m[2][2] + A.m[3][0] * B.m[2][3];
	m[2][1] = A.m[0][1] * B.m[2][0] + A.m[1][1] * B.m[2][1] + A.m[2][1] * B.m[2][2] + A.m[3][1] * B.m[2][3];
	m[2][2] = A.m[0][2] * B.m[2][0] + A.m[1][2] * B.m[2][1] + A.m[2][2] * B.m[2][2] + A.m[3][2] * B.m[2][3];
	m[2][3] = A.m[0][3] * B.m[2][0] + A.m[1][3] * B.m[2][1] + A.m[2][3] * B.m[2][2] + A.m[3][3] * B.m[2][3];

	m[3][0] = A.m[0][0] * B.m[3][0] + A.m[1][0] * B.m[3][1] + A.m[2][0] * B.m[3][2] + A.m[3][0] * B.m[3][3];
	m[3][1] = A.m[0][1] * B.m[3][0] + A.m[1][1] * B.m[3][1] + A.m[2][1] * B.m[3][2] + A.m[3][1] * B.m[3][3];
	m[3][2] = A.m[0][2] * B.m[3][0] + A.m[1][2] * B.m[3][1] + A.m[2][2] * B.m[3][2] + A.m[3][2] * B.m[3][3];
	m[3][3] = A.m[0][3] * B.m[3][0] + A.m[1][3] * B.m[3][1] + A.m[2][3] * B.m[3][2] + A.m[3][3] * B.m[3][3];
	return *this;
}

// Multiply RES = A[4x3]*B[4x3] (no projection), faster than ordinary multiply
template <typename T>
_matrix<T>& _matrix<T>::mul_43(const _matrix<T>& A, const _matrix<T>& B)
{
	VERIFY((this != &A) && (this != &B));
	m[0][0] = A.m[0][0] * B.m[0][0] + A.m[1][0] * B.m[0][1] + A.m[2][0] * B.m[0][2];
	m[0][1] = A.m[0][1] * B.m[0][0] + A.m[1][1] * B.m[0][1] + A.m[2][1] * B.m[0][2];
	m[0][2] = A.m[0][2] * B.m[0][0] + A.m[1][2] * B.m[0][1] + A.m[2][2] * B.m[0][2];
	m[0][3] = 0;

	m[1][0] = A.m[0][0] * B.m[1][0] + A.m[1][0] * B.m[1][1] + A.m[2][0] * B.m[1][2];
	m[1][1] = A.m[0][1] * B.m[1][0] + A.m[1][1] * B.m[1][1] + A.m[2][1] * B.m[1][2];
	m[1][2] = A.m[0][2] * B.m[1][0] + A.m[1][2] * B.m[1][1] + A.m[2][2] * B.m[1][2];
	m[1][3] = 0;

	m[2][0] = A.m[0][0] * B.m[2][0] + A.m[1][0] * B.m[2][1] + A.m[2][0] * B.m[2][2];
	m[2][1] = A.m[0][1] * B.m[2][0] + A.m[1][1] * B.m[2][1] + A.m[2][1] * B.m[2][2];
	m[2][2] = A.m[0][2] * B.m[2][0] + A.m[1][2] * B.m[2][1] + A.m[2][2] * B.m[2][2];
	m[2][3] = 0;

	m[3][0] = A.m[0][0] * B.m[3][0] + A.m[1][0] * B.m[3][1] + A.m[2][0] * B.m[3][2] + A.m[3][0];
	m[3][1] = A.m[0][1] * B.m[3][0] + A.m[1][1] * B.m[3][1] + A.m[2][1] * B.m[3][2] + A.m[3][1];
	m[3][2] = A.m[0][2] * B.m[3][0] + A.m[1][2] * B.m[3][1] + A.m[2][2] * B.m[3][2] + A.m[3][2];
	m[3][3] = 1;
	return *this;
}

template <typename T>
_matrix<T>& _matrix<T>::invert(const _matrix<T>& a)   // important: this is 4x3 invert, not the 4x4 one
{
	// faster than self-invert
	T fDetInv = (a._11 * (a._22 * a._33 - a._23 * a._32) -
		a._12 * (a._21 * a._33 - a._23 * a._31) +
		a._13 * (a._21 * a._32 - a._22 * a._31));

	VERIFY(_abs(fDetInv) > flt_zero);
	fDetInv = 1.0f / fDetInv;

	_11 = fDetInv * (a._22 * a._33 - a._23 * a._32);
	_12 = -fDetInv * (a._12 * a._33 - a._13 * a._32);
	_13 = fDetInv * (a._12 * a._23 - a._13 * a._22);
	_14 = 0.0f;

	_21 = -fDetInv * (a._21 * a._33 - a._23 * a._31);
	_22 = fDetInv * (a._11 * a._33 - a._13 * a._31);
	_23 = -fDetInv * (a._11 * a._23 - a._13 * a._21);
	_24 = 0.0f;

	_31 = fDetInv * (a._21 * a._32 - a._22 * a._31);
	_32 = -fDetInv * (a._11 * a._32 - a._12 * a._31);
	_33 = fDetInv * (a._11 * a._22 - a._12 * a._21);
	_34 = 0.0f;

	_41 = -(a._41 * _11 + a._42 * _21 + a._43 * _31);
	_42 = -(a._41 * _12 + a._42 * _22 + a._43 * _32);
	_43 = -(a._41 * _13 + a._42 * _23 + a._43 * _33);
	_44 = 1.0f;
	return *this;
}

template <typename T>
bool _matrix<T>::invert_b(const _matrix<T>& a)   // important: this is 4x3 invert, not the 4x4 one
{
	// faster than self-invert
	T fDetInv = (a._11 * (a._22 * a._33 - a._23 * a._32) -
		a._12 * (a._21 * a._33 - a._23 * a._31) +
		a._13 * (a._21 * a._32 - a._22 * a._31));

	if (_abs(fDetInv) <= flt_zero) return false;
	fDetInv = 1.0f / fDetInv;

	_11 = fDetInv * (a._22 * a._33 - a._23 * a._32);
	_12 = -fDetInv * (a._12 * a._33 - a._13 * a._32);
	_13 = fDetInv * (a._12 * a._23 - a._13 * a._22);
	_14 = 0.0f;

	_21 = -fDetInv * (a._21 * a._33 - a._23 * a._31);
	_22 = fDetInv * (a._11 * a._33 - a._13 * a._31);
	_23 = -fDetInv * (a._11 * a._23 - a._13 * a._21);
	_24 = 0.0f;

	_31 = fDetInv * (a._21 * a._32 - a._22 * a._31);
	_32 = -fDetInv * (a._11 * a._32 - a._12 * a._31);
	_33 = fDetInv * (a._11 * a._22 - a._12 * a._21);
	_34 = 0.0f;

	_41 = -(a._41 * _11 + a._42 * _21 + a._43 * _31);
	_42 = -(a._41 * _12 + a._42 * _22 + a._43 * _32);
	_43 = -(a._41 * _13 + a._42 * _23 + a._43 * _33);
	_44 = 1.0f;
	return true;
}

template <typename T>
_matrix<T>& _matrix<T>::invert_44(const _matrix<T>& a)
{
    const T &a11 = a._11, &a12 = a._12, &a13 = a._13, &a14 = a._14;
    const T &a21 = a._21, &a22 = a._22, &a23 = a._23, &a24 = a._24;
    const T &a31 = a._31, &a32 = a._32, &a33 = a._33, &a34 = a._34;
    const T &a41 = a._41, &a42 = a._42, &a43 = a._43, &a44 = a._44;

    T mn1 = a33 * a44 - a34 * a43;
    T mn2 = a32 * a44 - a34 * a42;
    T mn3 = a32 * a43 - a33 * a42;
    T mn4 = a31 * a44 - a34 * a41;
    T mn5 = a31 * a43 - a33 * a41;
    T mn6 = a31 * a42 - a32 * a41;

    T A11 = a22 * mn1 - a23 * mn2 + a24 * mn3;
    T A12 = -(a21 * mn1 - a23 * mn4 + a24 * mn5);
    T A13 = a21 * mn2 - a22 * mn4 + a24 * mn6;
    T A14 = -(a21 * mn3 - a22 * mn5 + a23 * mn6);

    T detInv = a11 * A11 + a12 * A12 + a13 * A13 + a14 * A14;
    VERIFY(_abs(detInv) > flt_zero);

    detInv = 1.f / detInv;

    _11 = detInv * A11;
    _12 = -detInv * (a12 * mn1 - a32 * (a13 * a44 - a43 * a14) + a42 * (a13 * a34 - a33 * a14));
    _13 = detInv * (a12 * (a23 * a44 - a43 * a24) - a22 * (a13 * a44 - a43 * a14) + a42 * (a13 * a24 - a23 * a14));
    _14 = -detInv * (a12 * (a23 * a34 - a33 * a24) - a22 * (a13 * a34 - a33 * a14) + a32 * (a13 * a24 - a23 * a14));

    _21 = detInv * A12;
    _22 = detInv * (a11 * mn1 - a31 * (a13 * a44 - a43 * a14) + a41 * (a13 * a34 - a33 * a14));
    _23 = -detInv * (a11 * (a23 * a44 - a43 * a24) - a21 * (a13 * a44 - a43 * a14) + a41 * (a13 * a24 - a23 * a14));
    _24 = detInv * (a11 * (a23 * a34 - a33 * a24) - a21 * (a13 * a34 - a33 * a14) + a31 * (a13 * a24 - a23 * a14));

    _31 = detInv * A13;
    _32 = -detInv * (a11 * (a32 * a44 - a42 * a34) - a31 * (a12 * a44 - a42 * a14) + a41 * (a12 * a34 - a32 * a14));
    _33 = detInv * (a11 * (a22 * a44 - a42 * a24) - a21 * (a12 * a44 - a42 * a14) + a41 * (a12 * a24 - a22 * a14));
    _34 = -detInv * (a11 * (a22 * a34 - a32 * a24) - a21 * (a12 * a34 - a32 * a14) + a31 * (a12 * a24 - a22 * a14));

    /*
        _11, _12, _13, _14;
        _21, _22, _23, _24;
        _31, _32, _33, _34;
        _41, _42, _43, _44;
    */

    _41 = detInv * A14;
    _42 = detInv * (a11 * (a32 * a43 - a42 * a33) - a31 * (a12 * a43 - a42 * a13) + a41 * (a12 * a33 - a32 * a13));
    _43 = -detInv * (a11 * (a22 * a43 - a42 * a23) - a21 * (a12 * a43 - a42 * a13) + a41 * (a12 * a23 - a22 * a13));
    _44 = detInv * (a11 * (a22 * a33 - a32 * a23) - a21 * (a12 * a33 - a32 * a13) + a31 * (a12 * a23 - a22 * a13));

    return *this;
}

template <typename T>
_matrix<T>& _matrix<T>::transpose(const _matrix<T>& matSource)
{
	_11 = matSource._11;
	_12 = matSource._21;
	_13 = matSource._31;
	_14 = matSource._41;
	_21 = matSource._12;
	_22 = matSource._22;
	_23 = matSource._32;
	_24 = matSource._42;
	_31 = matSource._13;
	_32 = matSource._23;
	_33 = matSource._33;
	_34 = matSource._43;
	_41 = matSource._14;
	_42 = matSource._24;
	_43 = matSource._34;
	_44 = matSource._44;
	return *this;
}

template <typename T>
_matrix<T>& _matrix<T>::rotateX(T Angle) // rotation about X axis
{
	T cosa = _cos(Angle);
	T sina = _sin(Angle);
	i.set(1, 0, 0);
	_14 = 0;
	j.set(0, cosa, sina);
	_24 = 0;
	k.set(0, -sina, cosa);
	_34 = 0;
	c.set(0, 0, 0);
	_44 = 1;
	return *this;
}

template <typename T>
_matrix<T>& _matrix<T>::rotateY(T Angle) // rotation about Y axis
{
	T cosa = _cos(Angle);
	T sina = _sin(Angle);
	i.set(cosa, 0, -sina);
	_14 = 0;
	j.set(0, 1, 0);
	_24 = 0;
	k.set(sina, 0, cosa);
	_34 = 0;
	c.set(0, 0, 0);
	_44 = 1;
	return *this;
}

template <typename T>
_matrix<T>& _matrix<T>::rotateZ(T Angle) // rotation about Z axis
{
	T cosa = _cos(Angle);
	T sina = _sin(Angle);
	i.set(cosa, sina, 0);
	_14 = 0;
	j.set(-sina, cosa, 0);
	_24 = 0;
	k.set(0, 0, 1);
	_34 = 0;
	c.set(0, 0, 0);
	_44 = 1;
	return *this;
}

template <typename T>
_matrix<T>& _matrix<T>::rotation(const Tvector& vdir, const Tvector& vnorm)
{
	Tvector vright;
	vright.crossproduct(vnorm, vdir).normalize();
	m[0][0] = vright.x;
	m[0][1] = vright.y;
	m[0][2] = vright.z;
	m[0][3] = 0;
	m[1][0] = vnorm.x;
	m[1][1] = vnorm.y;
	m[1][2] = vnorm.z;
	m[1][3] = 0;
	m[2][0] = vdir.x;
	m[2][1] = vdir.y;
	m[2][2] = vdir.z;
	m[2][3] = 0;
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;
	return *this;
}

template <typename T>
_matrix<T>& _matrix<T>::rotation(const Tvector& axis, T Angle)
{
	T Cosine = _cos(Angle);
	T Sine = _sin(Angle);
	m[0][0] = axis.x * axis.x + (1 - axis.x * axis.x) * Cosine;
	m[0][1] = axis.x * axis.y * (1 - Cosine) + axis.z * Sine;
	m[0][2] = axis.x * axis.z * (1 - Cosine) - axis.y * Sine;
	m[0][3] = 0;
	m[1][0] = axis.x * axis.y * (1 - Cosine) - axis.z * Sine;
	m[1][1] = axis.y * axis.y + (1 - axis.y * axis.y) * Cosine;
	m[1][2] = axis.y * axis.z * (1 - Cosine) + axis.x * Sine;
	m[1][3] = 0;
	m[2][0] = axis.x * axis.z * (1 - Cosine) + axis.y * Sine;
	m[2][1] = axis.y * axis.z * (1 - Cosine) - axis.x * Sine;
	m[2][2] = axis.z * axis.z + (1 - axis.z * axis.z) * Cosine;
	m[2][3] = 0;
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;
	return *this;
}

template <typename T>
_matrix<T>& _matrix<T>::mapXYZ() { i.set(1, 0, 0); _14 = 0; j.set(0, 1, 0); _24 = 0; k.set(0, 0, 1); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }
template <typename T>
_matrix<T>& _matrix<T>::mapXZY() { i.set(1, 0, 0); _14 = 0; j.set(0, 0, 1); _24 = 0; k.set(0, 1, 0); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }
template <typename T>
_matrix<T>& _matrix<T>::mapYXZ() { i.set(0, 1, 0); _14 = 0; j.set(1, 0, 0); _24 = 0; k.set(0, 0, 1); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }
template <typename T>
_matrix<T>& _matrix<T>::mapYZX() { i.set(0, 1, 0); _14 = 0; j.set(0, 0, 1); _24 = 0; k.set(1, 0, 0); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }
template <typename T>
_matrix<T>& _matrix<T>::mapZXY() { i.set(0, 0, 1); _14 = 0; j.set(1, 0, 0); _24 = 0; k.set(0, 1, 0); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }
template <typename T>
_matrix<T>& _matrix<T>::mapZYX() { i.set(0, 0, 1); _14 = 0; j.set(0, 1, 0); _24 = 0; k.set(1, 0, 0); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }

template <typename T>
_matrix<T>& _matrix<T>::mul(const _matrix<T>& A, T v)
{
	m[0][0] = A.m[0][0] * v;
	m[0][1] = A.m[0][1] * v;
	m[0][2] = A.m[0][2] * v;
	m[0][3] = A.m[0][3] * v;
	m[1][0] = A.m[1][0] * v;
	m[1][1] = A.m[1][1] * v;
	m[1][2] = A.m[1][2] * v;
	m[1][3] = A.m[1][3] * v;
	m[2][0] = A.m[2][0] * v;
	m[2][1] = A.m[2][1] * v;
	m[2][2] = A.m[2][2] * v;
	m[2][3] = A.m[2][3] * v;
	m[3][0] = A.m[3][0] * v;
	m[3][1] = A.m[3][1] * v;
	m[3][2] = A.m[3][2] * v;
	m[3][3] = A.m[3][3] * v;
	return *this;
}
template <typename T>
_matrix<T>& _matrix<T>::mul(T v)
{
	m[0][0] *= v;
	m[0][1] *= v;
	m[0][2] *= v;
	m[0][3] *= v;
	m[1][0] *= v;
	m[1][1] *= v;
	m[1][2] *= v;
	m[1][3] *= v;
	m[2][0] *= v;
	m[2][1] *= v;
	m[2][2] *= v;
	m[2][3] *= v;
	m[3][0] *= v;
	m[3][1] *= v;
	m[3][2] *= v;
	m[3][3] *= v;
	return *this;
}
template <typename T>
_matrix<T>& _matrix<T>::div(const _matrix<T>& A, T v)
{
	VERIFY(_abs(v) > 0.000001f);
	return mul(A, 1.0f / v);
}
template <typename T>
_matrix<T>& _matrix<T>::div(T v)
{
	VERIFY(_abs(v) > 0.000001f);
	return mul(1.0f / v);
}

template <typename T>
_matrix<T>& _matrix<T>::setHPB(T h, T p, T b)
{
	T _ch, _cp, _cb, _sh, _sp, _sb, _cc, _cs, _sc, _ss;

	_sh = _sin(h);
	_ch = _cos(h);
	_sp = _sin(p);
	_cp = _cos(p);
	_sb = _sin(b);
	_cb = _cos(b);
	_cc = _ch*_cb;
	_cs = _ch*_sb;
	_sc = _sh*_cb;
	_ss = _sh*_sb;

	i.set(_cc - _sp*_ss, -_cp*_sb, _sp*_cs + _sc);
	_14_ = 0;
	j.set(_sp*_sc + _cs, _cp*_cb, _ss - _sp*_cc);
	_24_ = 0;
	k.set(-_cp*_sh, _sp, _cp*_ch);
	_34_ = 0;
	c.set(0, 0, 0);
	_44_ = 1;
	return *this;
}

template <typename T>
void _matrix<T>::getHPB(T& h, T& p, T& b) const
{
	T cy = _sqrt(j.y*j.y + i.y*i.y);
	if (cy > 16.0f*type_epsilon<T>)
	{
		h = (T)-atan2(k.x, k.z);
		p = (T)-atan2(-k.y, cy);
		b = (T)-atan2(i.y, j.y);
	}
	else
	{
		h = (T)-atan2(-i.z, i.x);
		p = (T)-atan2(-k.y, cy);
		b = 0;
	}
}

// explicit instantiations

template Fmatrix& Fmatrix::rotation(const _quaternion<float>& Q);
template Dmatrix& Dmatrix::rotation(const _quaternion<double>& Q);
template Fmatrix& Fmatrix::mk_xform(const _quaternion<float>& Q, const Tvector& V);
template Dmatrix& Dmatrix::mk_xform(const _quaternion<double>& Q, const Tvector& V);
template Fmatrix& Fmatrix::identity();
template Dmatrix& Dmatrix::identity();
template Fmatrix& Fmatrix::mul(const Fmatrix& A, const Fmatrix& B);
template Dmatrix& Dmatrix::mul(const Dmatrix& A, const Dmatrix& B);
template Fmatrix& Fmatrix::mul_43(const Fmatrix& A, const Fmatrix& B);
template Dmatrix& Dmatrix::mul_43(const Dmatrix& A, const Dmatrix& B);
template Fmatrix& Fmatrix::invert(const Fmatrix& a);
template Dmatrix& Dmatrix::invert(const Dmatrix& a);
template bool     Fmatrix::invert_b(const Fmatrix& a);
template bool     Dmatrix::invert_b(const Dmatrix& a);
template Fmatrix& Fmatrix::invert_44(const Fmatrix& a);
template Dmatrix& Dmatrix::invert_44(const Dmatrix& a);
template Fmatrix& Fmatrix::transpose(const Fmatrix& matSource);
template Dmatrix& Dmatrix::transpose(const Dmatrix& matSource);
template Fmatrix& Fmatrix::rotateX(Fmatrix::TYPE Angle);
template Dmatrix& Dmatrix::rotateX(Dmatrix::TYPE Angle);
template Fmatrix& Fmatrix::rotateY(Fmatrix::TYPE Angle);
template Dmatrix& Dmatrix::rotateY(Dmatrix::TYPE Angle);
template Fmatrix& Fmatrix::rotateZ(Fmatrix::TYPE Angle);
template Dmatrix& Dmatrix::rotateZ(Dmatrix::TYPE Angle);
template Fmatrix& Fmatrix::rotation(const Tvector& vdir, const Tvector& vnorm);
template Dmatrix& Dmatrix::rotation(const Tvector& vdir, const Tvector& vnorm);
template Fmatrix& Fmatrix::rotation(const Tvector& axis, Fmatrix::TYPE  Angle);
template Dmatrix& Dmatrix::rotation(const Tvector& axis, Dmatrix::TYPE  Angle);

template Fmatrix& Fmatrix::mapXYZ();
template Fmatrix& Fmatrix::mapXZY();
template Fmatrix& Fmatrix::mapYXZ();
template Fmatrix& Fmatrix::mapYZX();
template Fmatrix& Fmatrix::mapZXY();
template Fmatrix& Fmatrix::mapZYX();
template Dmatrix& Dmatrix::mapXYZ();
template Dmatrix& Dmatrix::mapXZY();
template Dmatrix& Dmatrix::mapYXZ();
template Dmatrix& Dmatrix::mapYZX();
template Dmatrix& Dmatrix::mapZXY();
template Dmatrix& Dmatrix::mapZYX();

template Fmatrix& Fmatrix::mul(const Fmatrix& A, Fmatrix::TYPE v);
template Dmatrix& Dmatrix::mul(const Dmatrix& A, Dmatrix::TYPE v);
template Fmatrix& Fmatrix::mul(Fmatrix::TYPE v);
template Dmatrix& Dmatrix::mul(Dmatrix::TYPE v);
template Fmatrix& Fmatrix::div(const Fmatrix& A, Fmatrix::TYPE v);
template Dmatrix& Dmatrix::div(const Dmatrix& A, Dmatrix::TYPE v);
template Fmatrix& Fmatrix::div(Fmatrix::TYPE v);
template Dmatrix& Dmatrix::div(Dmatrix::TYPE v);

template Fmatrix& Fmatrix::setHPB(Fmatrix::TYPE h, Fmatrix::TYPE p, Fmatrix::TYPE b);
template Dmatrix& Dmatrix::setHPB(Dmatrix::TYPE h, Dmatrix::TYPE p, Dmatrix::TYPE b);
template void     Fmatrix::getHPB(Fmatrix::TYPE& h, Fmatrix::TYPE& p, Fmatrix::TYPE& b) const;
template void     Dmatrix::getHPB(Dmatrix::TYPE& h, Dmatrix::TYPE& p, Dmatrix::TYPE& b) const;
