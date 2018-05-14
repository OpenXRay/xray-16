// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_QUATERNION_H
#define NV_MATH_QUATERNION_H

#include <nvmath/nvmath.h>
#include <nvmath/Vector.h>

namespace nv
{

	class NVMATH_CLASS Quaternion
	{
	public:
		typedef Quaternion const & Arg;
		
		Quaternion();
		explicit Quaternion(zero_t);
		Quaternion(float x, float y, float z, float w);
		Quaternion(Vector4::Arg v);
		
		const Quaternion & operator=(Quaternion::Arg v);
		
		scalar x() const;
		scalar y() const;
		scalar z() const;
		scalar w() const;
		
		const Vector4 & asVector() const;
		Vector4 & asVector();
		
	private:
		Vector4 q;
	};

	inline Quaternion::Quaternion() {}
	inline Quaternion::Quaternion(zero_t) : q(zero) {}
	inline Quaternion::Quaternion(float x, float y, float z, float w) : q(x, y, z, w) {}
	inline Quaternion::Quaternion(Vector4::Arg v) : q(v) {}
	
	inline const Quaternion & Quaternion::operator=(Quaternion::Arg v) { q = v.q; return *this; }
	
	inline scalar Quaternion::x() const { return q.x(); }
	inline scalar Quaternion::y() const { return q.y(); }
	inline scalar Quaternion::z() const { return q.z(); }
	inline scalar Quaternion::w() const { return q.w(); }

	inline const Vector4 & Quaternion::asVector() const { return q; }
	inline Vector4 & Quaternion::asVector() { return q; }


	inline Quaternion mul(Quaternion::Arg a, Quaternion::Arg b)
	{
		// @@ Efficient SIMD implementation?
		return Quaternion(
			+ a.x() * b.w() + a.y()*b.z() - a.z()*b.y() + a.w()*b.x(),
			- a.x() * b.z() + a.y()*b.w() + a.z()*b.x() + a.w()*b.y(),
			+ a.x() * b.y() - a.y()*b.x() + a.z()*b.w() + a.w()*b.z(),
			- a.x() * b.x() - a.y()*b.y() - a.z()*b.z() + a.w()*b.w());
	}

	inline Quaternion scale(Quaternion::Arg q, float s)
	{
		return scale(q.asVector(), s);
	}
	inline Quaternion operator *(Quaternion::Arg q, float s)
	{
		return scale(q, s);
	}
	inline Quaternion operator *(float s, Quaternion::Arg q)
	{
		return scale(q, s);
	}

	inline Quaternion scale(Quaternion::Arg q, Vector4::Arg s)
	{
		return scale(q.asVector(), s);
	}
	/*inline Quaternion operator *(Quaternion::Arg q, Vector4::Arg s)
	{
		return scale(q, s);
	}
	inline Quaternion operator *(Vector4::Arg s, Quaternion::Arg q)
	{
		return scale(q, s);
	}*/

	inline Quaternion conjugate(Quaternion::Arg q)
	{
		return scale(q, Vector4(-1, -1, -1, 1));
	}

	inline float length(Quaternion::Arg q)
	{
		return length(q.asVector());
	}

	inline bool isNormalized(Quaternion::Arg q, float epsilon = NV_NORMAL_EPSILON)
	{
		return equal(length(q), 1, epsilon);
	}

	inline Quaternion normalize(Quaternion::Arg q, float epsilon = NV_EPSILON)
	{
		float l = length(q);
		nvDebugCheck(!isZero(l, epsilon));
		Quaternion n = scale(q, 1.0f / l);
		nvDebugCheck(isNormalized(n));
		return n;
	}

	inline Quaternion inverse(Quaternion::Arg q)
	{
		return conjugate(normalize(q));
	}

	/// Create a rotation quaternion for @a angle alpha around normal vector @a v.
	inline Quaternion axisAngle(Vector3::Arg v, float alpha)
	{
		float s = sinf(alpha * 0.5f);
		float c = cosf(alpha * 0.5f);
		return Quaternion(Vector4(v * s, c));
	}


} // nv namespace

#endif // NV_MATH_QUATERNION_H
