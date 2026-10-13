#include "pch.hpp"

#include "xrCore/_matrix.h"
#include "xrCore/_quaternion.h"
#include "xrCore/xrDebug_macros.h"
#include "xrCore/xrDebug.h"

#include <limits>

Fmatrix& Fmatrix::rotation(const Fquaternion& Q)
{
	float xx = Q.x*Q.x;
	float yy = Q.y*Q.y;
	float zz = Q.z*Q.z;
	float xy = Q.x*Q.y;
	float xz = Q.x*Q.z;
	float yz = Q.y*Q.z;
	float wx = Q.w*Q.x;
	float wy = Q.w*Q.y;
	float wz = Q.w*Q.z;

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

Fmatrix& Fmatrix::mk_xform(const Fquaternion& Q, const Fvector& V)
{
	float xx = Q.x*Q.x;
	float yy = Q.y*Q.y;
	float zz = Q.z*Q.z;
	float xy = Q.x*Q.y;
	float xz = Q.x*Q.z;
	float yz = Q.y*Q.z;
	float wx = Q.w*Q.x;
	float wy = Q.w*Q.y;
	float wz = Q.w*Q.z;

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

Fmatrix& Fmatrix::identity()
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

Fmatrix& Fmatrix::mul(const Fmatrix& A, const Fmatrix& B)
{
	VERIFY((this != &A) && (this != &B));
	__m128 a0 = _mm_loadu_ps(&A.m[0][0]);
	__m128 a1 = _mm_loadu_ps(&A.m[1][0]);
	__m128 a2 = _mm_loadu_ps(&A.m[2][0]);
	__m128 a3 = _mm_loadu_ps(&A.m[3][0]);
	for (int i = 0; i < 4; i++)
	{
		__m128 r = _mm_mul_ps(a0, _mm_set1_ps(B.m[i][0]));
		r = _mm_add_ps(r, _mm_mul_ps(a1, _mm_set1_ps(B.m[i][1])));
		r = _mm_add_ps(r, _mm_mul_ps(a2, _mm_set1_ps(B.m[i][2])));
		r = _mm_add_ps(r, _mm_mul_ps(a3, _mm_set1_ps(B.m[i][3])));
		_mm_storeu_ps(&m[i][0], r);
	}
	return *this;
}

Fmatrix& Fmatrix::mul_43(const Fmatrix& A, const Fmatrix& B)
{
	VERIFY((this != &A) && (this != &B));
	__m128 a0 = _mm_loadu_ps(&A.m[0][0]);
	__m128 a1 = _mm_loadu_ps(&A.m[1][0]);
	__m128 a2 = _mm_loadu_ps(&A.m[2][0]);
	__m128 a3 = _mm_loadu_ps(&A.m[3][0]);
	for (int i = 0; i < 3; i++)
	{
		__m128 r = _mm_mul_ps(a0, _mm_set1_ps(B.m[i][0]));
		r = _mm_add_ps(r, _mm_mul_ps(a1, _mm_set1_ps(B.m[i][1])));
		r = _mm_add_ps(r, _mm_mul_ps(a2, _mm_set1_ps(B.m[i][2])));
		_mm_storeu_ps(&m[i][0], r);
	}
	__m128 r = _mm_mul_ps(a0, _mm_set1_ps(B.m[3][0]));
	r = _mm_add_ps(r, _mm_mul_ps(a1, _mm_set1_ps(B.m[3][1])));
	r = _mm_add_ps(r, _mm_mul_ps(a2, _mm_set1_ps(B.m[3][2])));
	r = _mm_add_ps(r, a3);
	_mm_storeu_ps(&m[3][0], r);
	m[0][3] = 0; m[1][3] = 0; m[2][3] = 0; m[3][3] = 1;
	return *this;
}

Fmatrix& Fmatrix::invert(const Fmatrix& a)   // important: this is 4x3 invert, not the 4x4 one
{
	// faster than self-invert
	float fDetInv = (a._11 * (a._22 * a._33 - a._23 * a._32) -
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

bool Fmatrix::invert_b(const Fmatrix& a)   // important: this is 4x3 invert, not the 4x4 one
{
	// faster than self-invert
	float fDetInv = (a._11 * (a._22 * a._33 - a._23 * a._32) -
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

Fmatrix& Fmatrix::invert_44(const Fmatrix& a)
{
    const float &a11 = a._11, &a12 = a._12, &a13 = a._13, &a14 = a._14;
    const float &a21 = a._21, &a22 = a._22, &a23 = a._23, &a24 = a._24;
    const float &a31 = a._31, &a32 = a._32, &a33 = a._33, &a34 = a._34;
    const float &a41 = a._41, &a42 = a._42, &a43 = a._43, &a44 = a._44;

    float mn1 = a33 * a44 - a34 * a43;
    float mn2 = a32 * a44 - a34 * a42;
    float mn3 = a32 * a43 - a33 * a42;
    float mn4 = a31 * a44 - a34 * a41;
    float mn5 = a31 * a43 - a33 * a41;
    float mn6 = a31 * a42 - a32 * a41;

    float A11 = a22 * mn1 - a23 * mn2 + a24 * mn3;
    float A12 = -(a21 * mn1 - a23 * mn4 + a24 * mn5);
    float A13 = a21 * mn2 - a22 * mn4 + a24 * mn6;
    float A14 = -(a21 * mn3 - a22 * mn5 + a23 * mn6);

    float detInv = a11 * A11 + a12 * A12 + a13 * A13 + a14 * A14;
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

Fmatrix& Fmatrix::transpose(const Fmatrix& matSource)
{
	__m128 r0 = _mm_loadu_ps(&matSource.m[0][0]);
	__m128 r1 = _mm_loadu_ps(&matSource.m[1][0]);
	__m128 r2 = _mm_loadu_ps(&matSource.m[2][0]);
	__m128 r3 = _mm_loadu_ps(&matSource.m[3][0]);
	_MM_TRANSPOSE4_PS(r0, r1, r2, r3);
	_mm_storeu_ps(&m[0][0], r0);
	_mm_storeu_ps(&m[1][0], r1);
	_mm_storeu_ps(&m[2][0], r2);
	_mm_storeu_ps(&m[3][0], r3);
	return *this;
}

Fmatrix& Fmatrix::rotateX(float Angle) // rotation about X axis
{
	float cosa = _cos(Angle);
	float sina = _sin(Angle);
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

Fmatrix& Fmatrix::rotateY(float Angle) // rotation about Y axis
{
	float cosa = _cos(Angle);
	float sina = _sin(Angle);
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

Fmatrix& Fmatrix::rotateZ(float Angle) // rotation about Z axis
{
	float cosa = _cos(Angle);
	float sina = _sin(Angle);
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

Fmatrix& Fmatrix::rotation(const Fvector& vdir, const Fvector& vnorm)
{
	Fvector vright;
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

Fmatrix& Fmatrix::rotation(const Fvector& axis, float Angle)
{
	float Cosine = _cos(Angle);
	float Sine = _sin(Angle);
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

Fmatrix& Fmatrix::mapXYZ() { i.set(1, 0, 0); _14 = 0; j.set(0, 1, 0); _24 = 0; k.set(0, 0, 1); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }
Fmatrix& Fmatrix::mapXZY() { i.set(1, 0, 0); _14 = 0; j.set(0, 0, 1); _24 = 0; k.set(0, 1, 0); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }
Fmatrix& Fmatrix::mapYXZ() { i.set(0, 1, 0); _14 = 0; j.set(1, 0, 0); _24 = 0; k.set(0, 0, 1); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }
Fmatrix& Fmatrix::mapYZX() { i.set(0, 1, 0); _14 = 0; j.set(0, 0, 1); _24 = 0; k.set(1, 0, 0); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }
Fmatrix& Fmatrix::mapZXY() { i.set(0, 0, 1); _14 = 0; j.set(1, 0, 0); _24 = 0; k.set(0, 1, 0); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }
Fmatrix& Fmatrix::mapZYX() { i.set(0, 0, 1); _14 = 0; j.set(0, 1, 0); _24 = 0; k.set(1, 0, 0); _34 = 0; c.set(0, 0, 0); _44 = 1; return *this; }

Fmatrix& Fmatrix::mul(const Fmatrix& A, float v)
{
	__m128 sv = _mm_set1_ps(v);
	_mm_storeu_ps(&m[0][0], _mm_mul_ps(_mm_loadu_ps(&A.m[0][0]), sv));
	_mm_storeu_ps(&m[1][0], _mm_mul_ps(_mm_loadu_ps(&A.m[1][0]), sv));
	_mm_storeu_ps(&m[2][0], _mm_mul_ps(_mm_loadu_ps(&A.m[2][0]), sv));
	_mm_storeu_ps(&m[3][0], _mm_mul_ps(_mm_loadu_ps(&A.m[3][0]), sv));
	return *this;
}

Fmatrix& Fmatrix::mul(float v)
{
	__m128 sv = _mm_set1_ps(v);
	_mm_storeu_ps(&m[0][0], _mm_mul_ps(_mm_loadu_ps(&m[0][0]), sv));
	_mm_storeu_ps(&m[1][0], _mm_mul_ps(_mm_loadu_ps(&m[1][0]), sv));
	_mm_storeu_ps(&m[2][0], _mm_mul_ps(_mm_loadu_ps(&m[2][0]), sv));
	_mm_storeu_ps(&m[3][0], _mm_mul_ps(_mm_loadu_ps(&m[3][0]), sv));
	return *this;
}

Fmatrix& Fmatrix::div(const Fmatrix& A, float v)
{
	VERIFY(_abs(v) > 0.000001f);
	return mul(A, 1.0f / v);
}

Fmatrix& Fmatrix::div(float v)
{
	VERIFY(_abs(v) > 0.000001f);
	return mul(1.0f / v);
}


Fmatrix& Fmatrix::setHPB(float h, float p, float b)
{
	float _ch, _cp, _cb, _sh, _sp, _sb, _cc, _cs, _sc, _ss;

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

void Fmatrix::getHPB(float& h, float& p, float& b) const
{
	float cy = _sqrt(j.y*j.y + i.y*i.y);
	if (cy > 16.0f * type_epsilon<float>)
	{
		h = -atan2(k.x, k.z);
		p = -atan2(-k.y, cy);
		b = -atan2(i.y, j.y);
	}
	else
	{
		h = -atan2(-i.z, i.x);
		p = -atan2(-k.y, cy);
		b = 0;
	}
}
