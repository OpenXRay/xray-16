// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_VECTOR_H
#define NV_MATH_VECTOR_H

#include <nvmath/nvmath.h>
#include <nvcore/Containers.h> // min, max

namespace nv
{

enum zero_t { zero };
enum identity_t { identity };

// I should probably use templates.
typedef float scalar;

class NVMATH_CLASS Vector2
{
public:
	typedef Vector2 const & Arg;
	
	Vector2();
	explicit Vector2(zero_t);
	explicit Vector2(scalar f);
	Vector2(scalar x, scalar y);
	Vector2(Vector2::Arg v);
	
	const Vector2 & operator=(Vector2::Arg v);
	
	scalar x() const;
	scalar y() const;

	scalar component(uint idx) const;

	const scalar * ptr() const;

	void set(scalar x, scalar y);
	
	Vector2 operator-() const;
	void operator+=(Vector2::Arg v);
	void operator-=(Vector2::Arg v);
	void operator*=(scalar s);
	void operator*=(Vector2::Arg v);

	friend bool operator==(Vector2::Arg a, Vector2::Arg b);
	friend bool operator!=(Vector2::Arg a, Vector2::Arg b);

private:
	scalar m_x, m_y;
};


class NVMATH_CLASS Vector3
{
public:
	typedef Vector3 const & Arg;
	
	Vector3();
	explicit Vector3(zero_t);
	Vector3(scalar x, scalar y, scalar z);
	Vector3(Vector2::Arg v, scalar z);
	Vector3(Vector3::Arg v);
	
	const Vector3 & operator=(Vector3::Arg v);
	
	scalar x() const;
	scalar y() const;
	scalar z() const;

	const Vector2 & xy() const;

	scalar component(uint idx) const;

	const scalar * ptr() const;

	void set(scalar x, scalar y, scalar z);
	
	Vector3 operator-() const;
	void operator+=(Vector3::Arg v);
	void operator-=(Vector3::Arg v);
	void operator*=(scalar s);
	void operator/=(scalar s);
	void operator*=(Vector3::Arg v);

	friend bool operator==(Vector3::Arg a, Vector3::Arg b);
	friend bool operator!=(Vector3::Arg a, Vector3::Arg b);
	
private:
	scalar m_x, m_y, m_z;
};


class NVMATH_CLASS Vector4
{
public:
	typedef Vector4 const & Arg;
	
	Vector4();
	explicit Vector4(zero_t);
	Vector4(scalar x, scalar y, scalar z, scalar w);
	Vector4(Vector2::Arg v, scalar z, scalar w);
	Vector4(Vector3::Arg v, scalar w);
	Vector4(Vector4::Arg v);
//	Vector4(const Quaternion & v);
	
	const Vector4 & operator=(Vector4::Arg v);
	
	scalar x() const;
	scalar y() const;
	scalar z() const;
	scalar w() const;
	
	const Vector2 & xy() const;
	const Vector3 & xyz() const;

	scalar component(uint idx) const;

	const scalar * ptr() const;

	void set(scalar x, scalar y, scalar z, scalar w);
	
	Vector4 operator-() const;
	void operator+=(Vector4::Arg v);
	void operator-=(Vector4::Arg v);
	void operator*=(scalar s);
	void operator*=(Vector4::Arg v);
	
	friend bool operator==(Vector4::Arg a, Vector4::Arg b);
	friend bool operator!=(Vector4::Arg a, Vector4::Arg b);
	
private:
	scalar m_x, m_y, m_z, m_w;
};


// Vector2

inline Vector2::Vector2() {}
inline Vector2::Vector2(zero_t) : m_x(0.0f), m_y(0.0f) {}
inline Vector2::Vector2(scalar f) : m_x(f), m_y(f) {}
inline Vector2::Vector2(scalar x, scalar y) : m_x(x), m_y(y) {}
inline Vector2::Vector2(Vector2::Arg v) : m_x(v.x()), m_y(v.y()) {}

inline const Vector2 & Vector2::operator=(Vector2::Arg v)
{
	m_x = v.x();
	m_y = v.y();
	return *this;
}

inline scalar Vector2::x() const { return m_x; }
inline scalar Vector2::y() const { return m_y; }

inline scalar Vector2::component(uint idx) const
{
	nvDebugCheck(idx < 2);
	if (idx == 0) return x();
	if (idx == 1) return y();
	nvAssume(false);
	return 0.0f;
}

inline const scalar * Vector2::ptr() const
{
	return &m_x;
}

inline void Vector2::set(scalar x, scalar y)
{
	m_x = x;
	m_y = y;
}

inline Vector2 Vector2::operator-() const
{
	return Vector2(-m_x, -m_y);
}

inline void Vector2::operator+=(Vector2::Arg v)
{
	m_x += v.m_x;
	m_y += v.m_y;
}

inline void Vector2::operator-=(Vector2::Arg v)
{
	m_x -= v.m_x;
	m_y -= v.m_y;
}

inline void Vector2::operator*=(scalar s)
{
	m_x *= s;
	m_y *= s;
}

inline void Vector2::operator*=(Vector2::Arg v)
{
	m_x *= v.m_x;
	m_y *= v.m_y;
}

inline bool operator==(Vector2::Arg a, Vector2::Arg b)
{
	return a.m_x == b.m_x && a.m_y == b.m_y; 
}
inline bool operator!=(Vector2::Arg a, Vector2::Arg b)
{
	return a.m_x != b.m_x || a.m_y != b.m_y; 
}


// Vector3

inline Vector3::Vector3() {}
inline Vector3::Vector3(zero_t) : m_x(0.0f), m_y(0.0f), m_z(0.0f) {}
inline Vector3::Vector3(scalar x, scalar y, scalar z) : m_x(x), m_y(y), m_z(z) {}
inline Vector3::Vector3(Vector2::Arg v, scalar z) : m_x(v.x()), m_y(v.y()), m_z(z) {}
inline Vector3::Vector3(Vector3::Arg v) : m_x(v.x()), m_y(v.y()), m_z(v.z()) {}

inline const Vector3 & Vector3::operator=(Vector3::Arg v)
{
	m_x = v.m_x;
	m_y = v.m_y;
	m_z = v.m_z;
	return *this;
}
	
inline scalar Vector3::x() const { return m_x; } 
inline scalar Vector3::y() const { return m_y; }
inline scalar Vector3::z() const { return m_z; }
	
inline const Vector2 & Vector3::xy() const
{
	return *(Vector2 *)this;
}

inline scalar Vector3::component(uint idx) const
{
	nvDebugCheck(idx < 3);
	if (idx == 0) return x();
	if (idx == 1) return y();
	if (idx == 2) return z();
	nvAssume(false);
	return 0.0f;
}

inline const scalar * Vector3::ptr() const
{
	return &m_x;
}
	
inline void Vector3::set(scalar x, scalar y, scalar z)
{
	m_x = x;
	m_y = y;
	m_z = z;
}

inline Vector3 Vector3::operator-() const
{
	return Vector3(-m_x, -m_y, -m_z);
}

inline void Vector3::operator+=(Vector3::Arg v)
{
	m_x += v.m_x;
	m_y += v.m_y;
	m_z += v.m_z;
}

inline void Vector3::operator-=(Vector3::Arg v)
{
	m_x -= v.m_x;
	m_y -= v.m_y;
	m_z -= v.m_z;
}

inline void Vector3::operator*=(scalar s)
{
	m_x *= s;
	m_y *= s;
	m_z *= s;
}

inline void Vector3::operator/=(scalar s)
{
	float is = 1.0f / s;
	m_x *= is;
	m_y *= is;
	m_z *= is;
}

inline void Vector3::operator*=(Vector3::Arg v)
{
	m_x *= v.m_x;
	m_y *= v.m_y;
	m_z *= v.m_z;
}

inline bool operator==(Vector3::Arg a, Vector3::Arg b)
{
	return a.m_x == b.m_x && a.m_y == b.m_y && a.m_z == b.m_z; 
}
inline bool operator!=(Vector3::Arg a, Vector3::Arg b)
{
	return a.m_x != b.m_x || a.m_y != b.m_y || a.m_z != b.m_z; 
}


// Vector4

inline Vector4::Vector4() {}
inline Vector4::Vector4(zero_t) : m_x(0.0f), m_y(0.0f), m_z(0.0f), m_w(0.0f) {}
inline Vector4::Vector4(scalar x, scalar y, scalar z, scalar w) : m_x(x), m_y(y), m_z(z), m_w(w) {}
inline Vector4::Vector4(Vector2::Arg v, scalar z, scalar w) : m_x(v.x()), m_y(v.y()), m_z(z), m_w(w) {}
inline Vector4::Vector4(Vector3::Arg v, scalar w) : m_x(v.x()), m_y(v.y()), m_z(v.z()), m_w(w) {}
inline Vector4::Vector4(Vector4::Arg v) : m_x(v.x()), m_y(v.y()), m_z(v.z()), m_w(v.w()) {}

inline const Vector4 & Vector4::operator=(const Vector4 & v)
{
	m_x = v.m_x;
	m_y = v.m_y;
	m_z = v.m_z;
	m_w = v.m_w;
	return *this;
}

inline scalar Vector4::x() const { return m_x; }
inline scalar Vector4::y() const { return m_y; }
inline scalar Vector4::z() const { return m_z; }
inline scalar Vector4::w() const { return m_w; }

inline const Vector2 & Vector4::xy() const
{
	return *(Vector2 *)this;
}

inline const Vector3 & Vector4::xyz() const
{
	return *(Vector3 *)this;
}

inline scalar Vector4::component(uint idx) const
{
	nvDebugCheck(idx < 4);
	if (idx == 0) return x();
	if (idx == 1) return y();
	if (idx == 2) return z();
	if (idx == 3) return w();
	nvAssume(false);
	return 0.0f;
}

inline const scalar * Vector4::ptr() const
{
	return &m_x;
}

inline void Vector4::set(scalar x, scalar y, scalar z, scalar w)
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_w = w;
}

inline Vector4 Vector4::operator-() const
{
	return Vector4(-m_x, -m_y, -m_z, -m_w);
}

inline void Vector4::operator+=(Vector4::Arg v)
{
	m_x += v.m_x;
	m_y += v.m_y;
	m_z += v.m_z;
	m_w += v.m_w;
}

inline void Vector4::operator-=(Vector4::Arg v)
{
	m_x -= v.m_x;
	m_y -= v.m_y;
	m_z -= v.m_z;
	m_w -= v.m_w;
}

inline void Vector4::operator*=(scalar s)
{
	m_x *= s;
	m_y *= s;
	m_z *= s;
	m_w *= s;
}

inline void Vector4::operator*=(Vector4::Arg v)
{
	m_x *= v.m_x;
	m_y *= v.m_y;
	m_z *= v.m_z;
	m_w *= v.m_w;
}

inline bool operator==(Vector4::Arg a, Vector4::Arg b)
{
	return a.m_x == b.m_x && a.m_y == b.m_y && a.m_z == b.m_z && a.m_w == b.m_w; 
}
inline bool operator!=(Vector4::Arg a, Vector4::Arg b)
{
	return a.m_x != b.m_x || a.m_y != b.m_y || a.m_z != b.m_z || a.m_w != b.m_w; 
}



// Functions


// Vector2

inline Vector2 add(Vector2::Arg a, Vector2::Arg b)
{
	return Vector2(a.x() + b.x(), a.y() + b.y());
}
inline Vector2 operator+(Vector2::Arg a, Vector2::Arg b)
{
	return add(a, b);
}

inline Vector2 sub(Vector2::Arg a, Vector2::Arg b)
{
	return Vector2(a.x() - b.x(), a.y() - b.y());
}
inline Vector2 operator-(Vector2::Arg a, Vector2::Arg b)
{
	return sub(a, b);
}

inline Vector2 scale(Vector2::Arg v, scalar s)
{
	return Vector2(v.x() * s, v.y() * s);
}

inline Vector2 scale(Vector2::Arg v, Vector2::Arg s)
{
	return Vector2(v.x() * s.x(), v.y() * s.y());
}

inline Vector2 operator*(Vector2::Arg v, scalar s)
{
	return scale(v, s);
}

inline Vector2 operator*(Vector2::Arg v1, Vector2::Arg v2)
{
	return Vector2(v1.x()*v2.x(), v1.y()*v2.y());
}

inline Vector2 operator*(scalar s, Vector2::Arg v)
{
	return scale(v, s);
}

inline scalar dot(Vector2::Arg a, Vector2::Arg b)
{
	return a.x() * b.x() + a.y() * b.y();
}

inline scalar length_squared(Vector2::Arg v)
{
	return v.x() * v.x() + v.y() * v.y();
}

inline scalar length(Vector2::Arg v)
{
	return sqrtf(length_squared(v));
}

inline bool equal(Vector2::Arg v1, Vector2::Arg v2, float epsilon = NV_EPSILON)
{
	return equal(v1.x(), v2.x(), epsilon) && equal(v1.y(), v2.y(), epsilon);
}

inline Vector2 min(Vector2::Arg a, Vector2::Arg b)
{
	return Vector2(min(a.x(), b.x()), min(a.y(), b.y()));
}

inline Vector2 max(Vector2::Arg a, Vector2::Arg b)
{
	return Vector2(max(a.x(), b.x()), max(a.y(), b.y()));
}

inline bool isValid(Vector2::Arg v)
{
	return isFinite(v.x()) && isFinite(v.y());
}


// Vector3

inline Vector3 add(Vector3::Arg a, Vector3::Arg b)
{
	return Vector3(a.x() + b.x(), a.y() + b.y(), a.z() + b.z());
}
inline Vector3 add(Vector3::Arg a, float b)
{
	return Vector3(a.x() + b, a.y() + b, a.z() + b);
}
inline Vector3 operator+(Vector3::Arg a, Vector3::Arg b)
{
	return add(a, b);
}
inline Vector3 operator+(Vector3::Arg a, float b)
{
	return add(a, b);
}

inline Vector3 sub(Vector3::Arg a, Vector3::Arg b)
{
	return Vector3(a.x() - b.x(), a.y() - b.y(), a.z() - b.z());
}
inline Vector3 sub(Vector3::Arg a, float b)
{
	return Vector3(a.x() - b, a.y() - b, a.z() - b);
}
inline Vector3 operator-(Vector3::Arg a, Vector3::Arg b)
{
	return sub(a, b);
}
inline Vector3 operator-(Vector3::Arg a, float b)
{
	return sub(a, b);
}

inline Vector3 cross(Vector3::Arg a, Vector3::Arg b)
{
	return Vector3(a.y() * b.z() - a.z() * b.y(), a.z() * b.x() - a.x() * b.z(), a.x() * b.y() - a.y() * b.x());
}

inline Vector3 scale(Vector3::Arg v, scalar s)
{
	return Vector3(v.x() * s, v.y() * s, v.z() * s);
}

inline Vector3 scale(Vector3::Arg v, Vector3::Arg s)
{
	return Vector3(v.x() * s.x(), v.y() * s.y(), v.z() * s.z());
}

inline Vector3 operator*(Vector3::Arg v, scalar s)
{
	return scale(v, s);
}

inline Vector3 operator*(scalar s, Vector3::Arg v)
{
	return scale(v, s);
}

inline Vector3 operator*(Vector3::Arg v, Vector3::Arg s)
{
	return scale(v, s);
}

inline Vector3 operator/(Vector3::Arg v, scalar s)
{
	return scale(v, 1.0f/s);
}

inline Vector3 add_scaled(Vector3::Arg a, Vector3::Arg b, scalar s)
{
	return Vector3(a.x() + b.x() * s, a.y() + b.y() * s, a.z() + b.z() * s);
}

inline Vector3 lerp(Vector3::Arg v1, Vector3::Arg v2, scalar t)
{
	const scalar s = 1.0f - t;
	return Vector3(v1.x() * s + t * v2.x(), v1.y() * s + t * v2.y(), v1.z() * s + t * v2.z());
}

inline scalar dot(Vector3::Arg a, Vector3::Arg b)
{
	return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

inline scalar length_squared(Vector3::Arg v)
{
	return v.x() * v.x() + v.y() * v.y() + v.z() * v.z();
}

inline scalar length(Vector3::Arg v)
{
	return sqrtf(length_squared(v));
}

inline bool isNormalized(Vector3::Arg v, float epsilon = NV_NORMAL_EPSILON)
{
	return equal(length(v), 1, epsilon);
}

inline Vector3 normalize(Vector3::Arg v, float epsilon = NV_EPSILON)
{
	float l = length(v);
	nvDebugCheck(!isZero(l, epsilon));
	Vector3 n = scale(v, 1.0f / l);
	nvDebugCheck(isNormalized(n));
	return n;
}

inline Vector3 normalizeSafe(Vector3::Arg v, Vector3::Arg fallback, float epsilon = NV_EPSILON)
{
	float l = length(v);
	if (isZero(l, epsilon)) {
		return fallback;
	}
	return scale(v, 1.0f / l);
}

inline bool equal(Vector3::Arg v1, Vector3::Arg v2, float epsilon = NV_EPSILON)
{
	return equal(v1.x(), v2.x(), epsilon) && equal(v1.y(), v2.y(), epsilon) && equal(v1.z(), v2.z(), epsilon);
}

inline Vector3 min(Vector3::Arg a, Vector3::Arg b)
{
	return Vector3(min(a.x(), b.x()), min(a.y(), b.y()), min(a.z(), b.z()));
}

inline Vector3 max(Vector3::Arg a, Vector3::Arg b)
{
	return Vector3(max(a.x(), b.x()), max(a.y(), b.y()), max(a.z(), b.z()));
}

inline Vector3 clamp(Vector3::Arg v, float min, float max)
{
	return Vector3(clamp(v.x(), min, max), clamp(v.y(), min, max), clamp(v.z(), min, max));
}

inline bool isValid(Vector3::Arg v)
{
	return isFinite(v.x()) && isFinite(v.y()) && isFinite(v.z());
}

/*
Vector3 transform(Quaternion, vector3);
Vector3 transform_point(matrix34, vector3);
Vector3 transform_vector(matrix34, vector3);
Vector3 transform_point(matrix44, vector3);
Vector3 transform_vector(matrix44, vector3);
*/

// Vector4

inline Vector4 add(Vector4::Arg a, Vector4::Arg b)
{
	return Vector4(a.x() + b.x(), a.y() + b.y(), a.z() + b.z(), a.w() + b.w());
}
inline Vector4 operator+(Vector4::Arg a, Vector4::Arg b)
{
	return add(a, b);
}

inline Vector4 sub(Vector4::Arg a, Vector4::Arg b)
{
	return Vector4(a.x() - b.x(), a.y() - b.y(), a.z() - b.z(), a.w() - b.w());
}
inline Vector4 operator-(Vector4::Arg a, Vector4::Arg b)
{
	return sub(a, b);
}

inline Vector4 scale(Vector4::Arg v, scalar s)
{
	return Vector4(v.x() * s, v.y() * s, v.z() * s, v.w() * s);
}

inline Vector4 scale(Vector4::Arg v, Vector4::Arg s)
{
	return Vector4(v.x() * s.x(), v.y() * s.y(), v.z() * s.z(), v.w() * s.w());
}

inline Vector4 operator*(Vector4::Arg v, scalar s)
{
	return scale(v, s);
}

inline Vector4 operator*(scalar s, Vector4::Arg v)
{
	return scale(v, s);
}

inline Vector4 operator/(Vector4::Arg v, scalar s)
{
	return scale(v, 1.0f/s);
}

inline Vector4 add_scaled(Vector4::Arg a, Vector4::Arg b, scalar s)
{
	return Vector4(a.x() + b.x() * s, a.y() + b.y() * s, a.z() + b.z() * s, a.w() + b.w() * s);
}

inline scalar dot(Vector4::Arg a, Vector4::Arg b)
{
	return a.x() * b.x() + a.y() * b.y() + a.z() * b.z() + a.w() * b.w();
}

inline scalar length_squared(Vector4::Arg v)
{
	return v.x() * v.x() + v.y() * v.y() + v.z() * v.z() + v.w() * v.w();
}

inline scalar length(Vector4::Arg v)
{
	return sqrtf(length_squared(v));
}

inline bool isNormalized(Vector4::Arg v, float epsilon = NV_NORMAL_EPSILON)
{
	return equal(length(v), 1, epsilon);
}

inline Vector4 normalize(Vector4::Arg v, float epsilon = NV_EPSILON)
{
	float l = length(v);
	nvDebugCheck(!isZero(l, epsilon));
	Vector4 n = scale(v, 1.0f / l);
	nvDebugCheck(isNormalized(n));
	return n;
}

inline Vector4 normalizeSafe(Vector4::Arg v, Vector4::Arg fallback, float epsilon = NV_EPSILON)
{
	float l = length(v);
	if (isZero(l, epsilon)) {
		return fallback;
	}
	return scale(v, 1.0f / l);
}

inline bool equal(Vector4::Arg v1, Vector4::Arg v2, float epsilon = NV_EPSILON)
{
	return equal(v1.x(), v2.x(), epsilon) && equal(v1.y(), v2.y(), epsilon) && equal(v1.z(), v2.z(), epsilon) && equal(v1.w(), v2.w(), epsilon);
}

inline Vector4 min(Vector4::Arg a, Vector4::Arg b)
{
	return Vector4(min(a.x(), b.x()), min(a.y(), b.y()), min(a.z(), b.z()), min(a.w(), b.w()));
}

inline Vector4 max(Vector4::Arg a, Vector4::Arg b)
{
	return Vector4(max(a.x(), b.x()), max(a.y(), b.y()), max(a.z(), b.z()), max(a.w(), b.w()));
}

inline bool isValid(Vector4::Arg v)
{
	return isFinite(v.x()) && isFinite(v.y()) && isFinite(v.z()) && isFinite(v.w());
}



/*
vector4 transform(matrix34, vector4);
vector4 transform(matrix44, vector4);
*/

/*
Quaternion mul(Quaternion, Quaternion);   // rotational composition
Quaternion conjugate(Quaternion);
Quaternion inverse(Quaternion);
Quaternion axis_angle(const Vector3 & v, scalar s);
*/

/*
matrix34 add(matrix34, matrix34);            // note: implicit '1' stays as '1'
matrix34 operator+(matrix34, matrix34);
matrix34 sub(matrix34, matrix34);            // note: implicit '1' stays as '1'
matrix34 operator-(matrix34, matrix34);
matrix34 mul(matrix34, matrix34);
matrix34 operator*(matrix34, matrix34);
matrix34 mul(matrix34, quaternion4);         //  rotation multiplication
matrix34 operator*(matrix34, quaternion4);   //  rotation multiplication
matrix34 translation(vector3);
matrix34 rotation(quaternion4);
matrix34 rotation(vector3, scalar);          //  axis/angle

matrix44 add(matrix44, matrix44);
matrix44 operator+(matrix44, matrix44);
matrix44 sub(matrix44, matrix44);
matrix44 operator-(matrix44, matrix44);
matrix44 mul(matrix44, matrix44);
matrix44 operator*(matrix44, matrix44);
matrix44 mul(matrix44, quaternion4);         //  rotation multiplication
matrix44 operator*(matrix44, quaternion4);   //  rotation multiplication
matrix44 invert(matrix34);
matrix44 invert(matrix44);
matrix44 transpose(matrix34);
matrix44 transpose(matrix44);
*/

} // nv namespace

#endif // NV_MATH_VECTOR_H
