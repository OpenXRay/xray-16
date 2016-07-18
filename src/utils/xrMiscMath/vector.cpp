#include <limits>
#include "xrCore/vector.h"
#include "xrCore/_vector3d.h"
#include "xrCore/xrDebug_macros.h"
#include "xrCore/xrDebug.h"


// normalize angle (0..2PI)
float angle_normalize_always(float a)
{
	float div = a / PI_MUL_2;
	int rnd = (div > 0) ? iFloor(div) : iCeil(div);
	float frac = div - rnd;
	if (frac < 0) frac += 1.f;
	return frac * PI_MUL_2;
}

// normalize angle (0..2PI)
float angle_normalize(float a)
{
	if (a >= 0 && a <= PI_MUL_2) return a;
	return angle_normalize_always(a);
}

// -PI .. +PI
float angle_normalize_signed(float a)
{
	if (a >= (-PI) && a <= PI) return a;
	float angle = angle_normalize_always(a);
	if (angle > PI) angle -= PI_MUL_2;
	return angle;
}

float angle_difference_signed(float a, float b)
{
	float diff = angle_normalize_signed(a) - angle_normalize_signed(b);
	if (diff > 0)
	{
		if (diff > PI)
			diff -= PI_MUL_2;
	}
	else
	{
		if (diff < -PI)
			diff += PI_MUL_2;
	}
	return diff;
}

// 0..PI
float angle_difference(float a, float b)
{
	return _abs(angle_difference_signed(a, b));
}

bool are_ordered(float const value0, float const value1, float const value2)
{
	if ((value1 >= value0) && (value1 <= value2))
		return true;

	if ((value1 <= value0) && (value1 >= value2))
		return true;

	return false;
}

bool is_between(float const value, float const left, float const right)
{
	return are_ordered(left, value, right);
}

// c=current, t=target, s=speed, dt=dt
bool angle_lerp(float& c, float t, float s, float dt)
{
	float const before = c;
	float diff = t - c;
	if (diff > 0)
	{
		if (diff > PI)
			diff -= PI_MUL_2;
	}
	else
	{
		if (diff < -PI)
			diff += PI_MUL_2;
	}
	float diff_a = _abs(diff);

	if (diff_a < EPS_S)
		return true;

	float mot = s*dt;
	if (mot > diff_a) mot = diff_a;
	c += (diff / diff_a)*mot;

	if (is_between(c, before, t))
		return false;

	if (c < 0)
		c += PI_MUL_2;
	else if (c > PI_MUL_2)
		c -= PI_MUL_2;

	return false;
}

// Just lerp :) expects normalized angles in range [0..2PI)
float angle_lerp(float A, float B, float f)
{
	float diff = B - A;
	if (diff > PI) diff -= PI_MUL_2;
	else if (diff < -PI) diff += PI_MUL_2;

	return A + diff*f;
}

float angle_inertion(float src, float tgt, float speed, float clmp, float dt)
{
	float a = angle_normalize_signed(tgt);
	angle_lerp(src, a, speed, dt);
	src = angle_normalize_signed(src);
	float dH = angle_difference_signed(src, a);
	float dCH = clampr(dH, -clmp, clmp);
	src -= dH - dCH;
	return src;
}

float angle_inertion_var(float src, float tgt, float min_speed, float max_speed, float clmp, float dt)
{
	tgt = angle_normalize_signed(tgt);
	src = angle_normalize_signed(src);
	float speed = _abs((max_speed - min_speed)*angle_difference(tgt, src) / clmp) + min_speed;
	angle_lerp(src, tgt, speed, dt);
	src = angle_normalize_signed(src);
	float dH = angle_difference_signed(src, tgt);
	float dCH = clampr(dH, -clmp, clmp);
	src -= dH - dCH;
	return src;
}

double rsqrt(double v) { return 1.0 / _sqrt(v); }

//////////////////////////////////////////////////////////////////

template <typename T>
_vector3<T>& _vector3<T>::set_length(T l)
{
	mul(l / magnitude());
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::align()
{
	y = 0;
	if (_abs(z) >= _abs(x)) { z /= _abs(z ? z : 1); x = 0; }
	else { x /= _abs(x); z = 0; }
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::squeeze(T Epsilon)
{
	if (_abs(x) < Epsilon) x = 0;
	if (_abs(y) < Epsilon) y = 0;
	if (_abs(z) < Epsilon) z = 0;
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::clamp(const _vector3<T>& min, const _vector3<T>& max)
{
	::clamp(x, min.x, max.x);
	::clamp(y, min.y, max.y);
	::clamp(z, min.z, max.z);
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::clamp(const _vector3<T>& _v)
{
	_vector3<T> v;
	v.x = _abs(_v.x);
	v.y = _abs(_v.y);
	v.z = _abs(_v.z);
	::clamp(x, -v.x, v.x);
	::clamp(y, -v.y, v.y);
	::clamp(z, -v.z, v.z);
	return *this;
}

// Interpolate vectors (inertion)
template <typename T>
_vector3<T>& _vector3<T>::inertion(const _vector3<T>& p, T v)
{
	const T inv = 1.f - v;
	x = v*x + inv*p.x;
	y = v*y + inv*p.y;
	z = v*z + inv*p.z;
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::average(const _vector3<T>& p)
{
	x = (x + p.x)*0.5f;
	y = (y + p.y)*0.5f;
	z = (z + p.z)*0.5f;
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::average(const _vector3<T>& p1, const _vector3<T>& p2)
{
	x = (p1.x + p2.x)*0.5f;
	y = (p1.y + p2.y)*0.5f;
	z = (p1.z + p2.z)*0.5f;
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::lerp(const _vector3<T>& p1, const _vector3<T>& p2, T t)
{
	const T invt = 1.f - t;
	x = p1.x*invt + p2.x*t;
	y = p1.y*invt + p2.y*t;
	z = p1.z*invt + p2.z*t;
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::mad(const _vector3<T>& d, T m)
{
	x += d.x*m;
	y += d.y*m;
	z += d.z*m;
	return *this;
}
template <typename T>
_vector3<T>& _vector3<T>::mad(const _vector3<T>& p, const _vector3<T>& d, T m)
{
	x = p.x + d.x*m;
	y = p.y + d.y*m;
	z = p.z + d.z*m;
	return *this;
}
template <typename T>
_vector3<T>& _vector3<T>::mad(const _vector3<T>& d, const _vector3<T>& s)
{
	x += d.x*s.x;
	y += d.y*s.y;
	z += d.z*s.z;
	return *this;
}
template <typename T>
_vector3<T>& _vector3<T>::mad(const _vector3<T>& p, const _vector3<T>& d, const _vector3<T>& s)
{
	x = p.x + d.x*s.x;
	y = p.y + d.y*s.y;
	z = p.z + d.z*s.z;
	return *this;
}

template <typename T>
T _vector3<T>::square_magnitude() const
{
	return x*x + y*y + z*z;
}

template <typename T>
T _vector3<T>::magnitude() const
{
	return _sqrt(square_magnitude());
}

template <typename T>
T _vector3<T>::normalize_magn()
{
	VERIFY(square_magnitude() > std::numeric_limits<T>::min());
	const T len = magnitude();
	const T inv_len = T(1) / len;
	x *= inv_len;
	y *= inv_len;
	z *= inv_len;
	return len;
}

template <typename T>
_vector3<T>& _vector3<T>::normalize()
{
	VERIFY(square_magnitude() > std::numeric_limits<T>::min());
	T mag = _sqrt(T(1) / (x*x + y*y + z*z));
	x *= mag;
	y *= mag;
	z *= mag;
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::normalize_safe()
{
	T magnitude = x*x + y*y + z*z;
	if (magnitude > std::numeric_limits<T>::min())
	{
		magnitude = _sqrt(1 / magnitude);
		x *= magnitude;
		y *= magnitude;
		z *= magnitude;
	}
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::normalize(const _vector3<T>& v)
{
	VERIFY((v.x*v.x + v.y*v.y + v.z*v.z) > flt_zero);
	T mag = _sqrt(1 / (v.x*v.x + v.y*v.y + v.z*v.z));
	x = v.x*mag;
	y = v.y*mag;
	z = v.z*mag;
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::normalize_safe(const _vector3<T>& v)
{
	T magnitude = v.x*v.x + v.y*v.y + v.z*v.z;
	if (magnitude > std::numeric_limits<T>::min())
	{
		magnitude = _sqrt(1 / magnitude);
		x = v.x*magnitude;
		y = v.y*magnitude;
		z = v.z*magnitude;
	}
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::random_dir(CRandom& R)
{
	//z = R.randF(-1,1);
	z = _cos(R.randF(PI));
	T a = R.randF(PI_MUL_2);
	T r = _sqrt(1 - z*z);
	T sa = _sin(a);
	T ca = _cos(a);
	x = r * ca;
	y = r * sa;
	return *this;
};

template <typename T>
_vector3<T>& _vector3<T>::random_dir(const _vector3<T>& ConeAxis, float ConeAngle, CRandom& R)
{
	_vector3<T> rnd;
	rnd.random_dir(R);
	mad(ConeAxis, rnd, R.randF(tanf(ConeAngle)));
	normalize();
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::random_point(const _vector3<T>& BoxSize, CRandom& R)
{
	x = R.randFs(BoxSize.x);
	y = R.randFs(BoxSize.y);
	z = R.randFs(BoxSize.z);
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::random_point(T r, CRandom& R)
{
	random_dir(R);
	mul(R.randF(r));
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::crossproduct(const _vector3<T>& v1, const _vector3<T>& v2)
{
	x = v1.y * v2.z - v1.z * v2.y;
	y = v1.z * v2.x - v1.x * v2.z;
	z = v1.x * v2.y - v1.y * v2.x;
	return *this;
}

template <typename T>
T _vector3<T>::distance_to_xz(const Self& v) const
{
	return _sqrt((x - v.x)*(x - v.x) + (z - v.z)*(z - v.z));
}

template <typename T>
T _vector3<T>::distance_to_xz_sqr(const Self& v) const
{
	return (x - v.x)*(x - v.x) + (z - v.z)*(z - v.z);
}

template <typename T>
T _vector3<T>::distance_to_sqr(const Self& v) const
{
	return (x - v.x)*(x - v.x) + (y - v.y)*(y - v.y) + (z - v.z)*(z - v.z);
}

template <typename T>
T _vector3<T>::distance_to(const Self& v) const
{
	return _sqrt(distance_to_sqr(v));
}

template <typename T>
_vector3<T>& _vector3<T>::setHP(T h, T p)
{
	T _ch = _cos(h), _cp = _cos(p), _sh = _sin(h), _sp = _sin(p);
	x = -_cp*_sh;
	y = _sp;
	z = _cp*_ch;
	return *this;
}

template <typename T>
void _vector3<T>::getHP(T& h, T& p) const
{
	float hyp;

	if (fis_zero(x) && fis_zero(z))
	{
		h = 0.0f;
		if (!fis_zero(float(y))) p = (y > 0.0f) ? PI_DIV_2 : -PI_DIV_2;
		else p = 0.0f;
	}
	else
	{
		if (fis_zero(z)) h = (x > 0.0f) ? -PI_DIV_2 : PI_DIV_2;
		else if (z < 0.0f) h = -(atanf(x / z) - PI);
		else h = -atanf(x / z);
		hyp = _sqrt(x*x + z*z);
		if (fis_zero(float(hyp))) p = (y > 0.0f) ? PI_DIV_2 : -PI_DIV_2;
		else p = atanf(y / hyp);
	}
}

template <typename T>
float _vector3<T>::getH() const
{
	if (fis_zero(x) && fis_zero(z))
	{
		return 0.0f;
	}
	else
	{
		if (fis_zero(z)) return (x > 0.0f) ? -PI_DIV_2 : PI_DIV_2;
		else if (z < 0.0f) return -(atanf(x / z) - PI);
		else return -atanf(x / z);
	}
}

template <typename T>
float _vector3<T>::getP() const
{
	if (fis_zero(x) && fis_zero(z))
	{
		if (!fis_zero(float(y))) return (y > 0.0f) ? PI_DIV_2 : -PI_DIV_2;
		else return 0.0f;
	}
	else
	{
		float hyp = _sqrt(x*x + z*z);
		if (fis_zero(float(hyp))) return (y > 0.0f) ? PI_DIV_2 : -PI_DIV_2;
		else return atanf(y / hyp);
	}
}

template <typename T>
_vector3<T>& _vector3<T>::reflect(const _vector3<T>& dir, const _vector3<T>& norm)
{
	return mad(dir, norm, -2 * dir.dotproduct(norm));
}
template <typename T>
_vector3<T>& _vector3<T>::slide(const _vector3<T>& dir, const _vector3<T>& norm)
{
	// non normalized
	return mad(dir, norm, -dir.dotproduct(norm));
}

template <typename T>
void _vector3<T>::generate_orthonormal_basis(const _vector3<T>& dir, _vector3<T>& up, _vector3<T>& right)
{
	T fInvLength;

	if (_abs(dir.x) >= _abs(dir.y))
	{
		// W.x or W.z is the largest magnitude component, swap them
		fInvLength = 1.f / _sqrt(dir.x*dir.x + dir.z*dir.z);
		up.x = -dir.z*fInvLength;
		up.y = 0.0f;
		up.z = +dir.x*fInvLength;
	}
	else
	{
		// W.y or W.z is the largest magnitude component, swap them
		fInvLength = 1.f / _sqrt(dir.y*dir.y + dir.z*dir.z);
		up.x = 0.0f;
		up.y = +dir.z*fInvLength;
		up.z = -dir.y*fInvLength;
	}

	right.crossproduct(up, dir); //. <->
}

template <typename T>
void _vector3<T>::generate_orthonormal_basis_normalized(_vector3<T>& dir, _vector3<T>& up, _vector3<T>& right)
{
	T fInvLength;
	dir.normalize();
	if (fsimilar(dir.y, 1.f, EPS))
	{
		up.set(0.f, 0.f, 1.f);
		fInvLength = 1.f / _sqrt(dir.x*dir.x + dir.y*dir.y);
		// cross (up,dir) and normalize (right)
		right.x = -dir.y * fInvLength;
		right.y = dir.x * fInvLength;
		right.z = 0.f;
		// cross (dir,right)
		up.x = -dir.z * right.y;
		up.y = dir.z * right.x;
		up.z = dir.x * right.y - dir.y * right.x;
	}
	else
	{
		up.set(0.f, 1.f, 0.f);
		fInvLength = 1.f / _sqrt(dir.x*dir.x + dir.z*dir.z);
		// cross (up,dir) and normalize (right)
		right.x = dir.z * fInvLength;
		right.y = 0.f;
		right.z = -dir.x * fInvLength;
		// cross (dir,right)
		up.x = dir.y * right.z;
		up.y = dir.z * right.x - dir.x * right.z;
		up.z = -dir.y * right.x;
	}
}

// Barycentric coords
template <typename T>
_vector3<T>& _vector3<T>::from_bary(const _vector3<T>& V1, const _vector3<T>& V2, const _vector3<T>& V3, T u, T v, T w)
{
	x = V1.x*u + V2.x*v + V3.x*w;
	y = V1.y*u + V2.y*v + V3.y*w;
	z = V1.z*u + V2.z*v + V3.z*w;
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::from_bary(const _vector3<T>& V1, const _vector3<T>& V2, const _vector3<T>& V3, const _vector3<T>& B)
{
	from_bary(V1, V2, V3, B.x, B.y, B.z);
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::from_bary4(const _vector3<T>& V1, const _vector3<T>& V2, const _vector3<T>& V3, const _vector3<T>& V4, T u, T v, T w, T t)
{
	x = V1.x*u + V2.x*v + V3.x*w + V4.x*t;
	y = V1.y*u + V2.y*v + V3.y*w + V4.y*t;
	z = V1.z*u + V2.z*v + V3.z*w + V4.z*t;
	return *this;
}

template <typename T>
_vector3<T>& _vector3<T>::mknormal_non_normalized(const _vector3<T>& p0, const _vector3<T>& p1, const _vector3<T>& p2)
{
	_vector3<T> v01, v12;
	v01.sub(p1, p0);
	v12.sub(p2, p1);
	crossproduct(v01, v12);
	return *this;
};

template <typename T>
_vector3<T>& _vector3<T>::mknormal(const _vector3<T>& p0, const _vector3<T>& p1, const _vector3<T>& p2)
{
	mknormal_non_normalized(p0, p1, p2);
	normalize_safe();
	return *this;
};


// instantiations of the previous methods, for float and double

template Fvector& Fvector::set_length(Fvector::TYPE l);
template Dvector& Dvector::set_length(Dvector::TYPE l);
template Fvector& Fvector::align();
template Dvector& Dvector::align();
template Fvector& Fvector::squeeze(Fvector::TYPE Epsilon);
template Dvector& Dvector::squeeze(Dvector::TYPE Epsilon);
template Fvector& Fvector::clamp(const Fvector& min, const Fvector& max);
template Dvector& Dvector::clamp(const Dvector& min, const Dvector& max);
template Fvector& Fvector::clamp(const Fvector& _v);
template Dvector& Dvector::clamp(const Dvector& _v);
template Fvector& Fvector::inertion(const Fvector& p, Fvector::TYPE v);
template Dvector& Dvector::inertion(const Dvector& p, Dvector::TYPE v);
template Fvector& Fvector::average(const Fvector& p);
template Dvector& Dvector::average(const Dvector& p);
template Fvector& Fvector::average(const Fvector& p1, const Fvector& p2);
template Dvector& Dvector::average(const Dvector& p1, const Dvector& p2);
template Fvector& Fvector::lerp(const Fvector& p1, const Fvector& p2, Fvector::TYPE t);
template Dvector& Dvector::lerp(const Dvector& p1, const Dvector& p2, Dvector::TYPE t);
template Fvector& Fvector::mad(const Fvector& d, Fvector::TYPE m);
template Dvector& Dvector::mad(const Dvector& d, Dvector::TYPE m);
template Fvector& Fvector::mad(const Fvector& p, const Fvector& d, Fvector::TYPE m);
template Dvector& Dvector::mad(const Dvector& p, const Dvector& d, Dvector::TYPE m);
template Fvector& Fvector::mad(const Fvector& d, const Fvector& s);
template Dvector& Dvector::mad(const Dvector& d, const Dvector& s);
template Fvector& Fvector::mad(const Fvector& p, const Fvector& d, const Fvector& s);
template Dvector& Dvector::mad(const Dvector& p, const Dvector& d, const Dvector& s);

template Fvector::TYPE Fvector::square_magnitude() const;
template Dvector::TYPE Dvector::square_magnitude() const;
template Fvector::TYPE Fvector::magnitude() const;
template Dvector::TYPE Dvector::magnitude() const;
template Fvector::TYPE Fvector::normalize_magn();
template Dvector::TYPE Dvector::normalize_magn();

template Fvector& Fvector::normalize();
template Dvector& Dvector::normalize();
template Fvector& Fvector::normalize_safe();
template Dvector& Dvector::normalize_safe();
template Fvector& Fvector::normalize(const Fvector& v);
template Dvector& Dvector::normalize(const Dvector& v);
template Fvector& Fvector::normalize_safe(const Fvector& v);
template Dvector& Dvector::normalize_safe(const Dvector& v);
template Fvector& Fvector::random_dir(CRandom& R);
template Dvector& Dvector::random_dir(CRandom& R);
template Fvector& Fvector::random_dir(const Fvector& ConeAxis, float ConeAngle, CRandom& R);
template Dvector& Dvector::random_dir(const Dvector& ConeAxis, float ConeAngle, CRandom& R);
template Fvector& Fvector::random_point(const Fvector& BoxSize, CRandom& R);
template Dvector& Dvector::random_point(const Dvector& BoxSize, CRandom& R);
template Fvector& Fvector::random_point(Fvector::TYPE r, CRandom& R);
template Dvector& Dvector::random_point(Dvector::TYPE r, CRandom& R);
template Fvector& Fvector::crossproduct(const Fvector& v1, const Fvector& v2);
template Dvector& Dvector::crossproduct(const Dvector& v1, const Dvector& v2);

template Fvector::TYPE Fvector::distance_to_xz(const Fvector& v) const;
template Dvector::TYPE Dvector::distance_to_xz(const Dvector& v) const;
template Fvector::TYPE Fvector::distance_to_xz_sqr(const Fvector& v) const;
template Dvector::TYPE Dvector::distance_to_xz_sqr(const Dvector& v) const;
template Fvector::TYPE Fvector::distance_to_sqr(const Fvector& v) const;
template Dvector::TYPE Dvector::distance_to_sqr(const Dvector& v) const;
template Fvector::TYPE Fvector::distance_to(const Fvector& v) const;
template Dvector::TYPE Dvector::distance_to(const Dvector& v) const;

template Fvector& Fvector::from_bary(const Fvector& V1, const Fvector& V2, const Fvector& V3, Fvector::TYPE u, Fvector::TYPE v, Fvector::TYPE w);
template Dvector& Dvector::from_bary(const Dvector& V1, const Dvector& V2, const Dvector& V3, Dvector::TYPE u, Dvector::TYPE v, Dvector::TYPE w);
template Fvector& Fvector::from_bary(const Fvector& V1, const Fvector& V2, const Fvector& V3, const Fvector& B);
template Dvector& Dvector::from_bary(const Dvector& V1, const Dvector& V2, const Dvector& V3, const Dvector& B);
template Fvector& Fvector::from_bary4(const Fvector& V1, const Fvector& V2, const Fvector& V3, const Fvector& V4, Fvector::TYPE u, Fvector::TYPE v, Fvector::TYPE w, Fvector::TYPE t);
template Dvector& Dvector::from_bary4(const Dvector& V1, const Dvector& V2, const Dvector& V3, const Dvector& V4, Dvector::TYPE u, Dvector::TYPE v, Dvector::TYPE w, Dvector::TYPE t);

template Fvector& Fvector::mknormal_non_normalized(const Fvector& p0, const Fvector& p1, const Fvector& p2);
template Dvector& Dvector::mknormal_non_normalized(const Dvector& p0, const Dvector& p1, const Dvector& p2);
template Fvector& Fvector::mknormal(const Fvector& p0, const Fvector& p1, const Fvector& p2);
template Dvector& Dvector::mknormal(const Dvector& p0, const Dvector& p1, const Dvector& p2);

template Fvector& Fvector::setHP(Fvector::TYPE h, Fvector::TYPE p);
template Dvector& Dvector::setHP(Dvector::TYPE h, Dvector::TYPE p);
template void Fvector::getHP(Fvector::TYPE& h, Fvector::TYPE& p) const;
template void Dvector::getHP(Dvector::TYPE& h, Dvector::TYPE& p) const;
template float Fvector::getH() const;
template float Dvector::getH() const;
template float Fvector::getP() const;
template float Dvector::getP() const;

template Fvector& Fvector::reflect(const Fvector& dir, const Fvector& norm);
template Dvector& Dvector::reflect(const Dvector& dir, const Dvector& norm);
template Fvector& Fvector::slide(const Fvector& dir, const Fvector& norm);
template Dvector& Dvector::slide(const Dvector& dir, const Dvector& norm);

template void Fvector::generate_orthonormal_basis(const Fvector& dir, Fvector& up, Fvector& right);
template void Dvector::generate_orthonormal_basis(const Dvector& dir, Dvector& up, Dvector& right);
template void Fvector::generate_orthonormal_basis_normalized(Fvector& dir, Fvector& up, Fvector& right);
template void Dvector::generate_orthonormal_basis_normalized(Dvector& dir, Dvector& up, Dvector& right);

