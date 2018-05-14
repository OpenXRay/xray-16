// This code is in the public domain -- Ignacio Castaño <castanyo@yahoo.es>

#ifndef NV_MATH_TRIANGLE_H
#define NV_MATH_TRIANGLE_H

#include <nvmath/nvmath.h>
#include <nvmath/Vector.h>
#include <nvmath/Box.h>

namespace nv
{

	/// Triangle class with three vertices.
	class Triangle
	{
	public:
		Triangle() {};

		Triangle(Vector3::Arg v0, Vector3::Arg v1, Vector3::Arg v2)
		{
			v[0] = v0;
			v[1] = v1;
			v[2] = v2;
		}

		/// Get the bounds of the triangle.
		Box bounds() const
		{
			Box bounds;
			bounds.clearBounds();
			bounds.addPointToBounds(v[0]);
			bounds.addPointToBounds(v[1]);
			bounds.addPointToBounds(v[2]);
			return bounds;
		}

		Vector4 plane() const
		{
			Vector3 n = cross(v[1]-v[0], v[2]-v[0]);
			return Vector4(n, dot(n, v[0]));
		}

		Vector3 v[3];
	};


	// Tomas Akenine-Möller box-triangle test.
	NVMATH_API bool triBoxOverlap(Vector3::Arg boxcenter, Vector3::Arg boxhalfsize, const Triangle & triangle);
	NVMATH_API bool triBoxOverlapNoBounds(Vector3::Arg boxcenter, Vector3::Arg boxhalfsize, const Triangle & triangle);


	// Moller ray triangle test.
	NVMATH_API bool rayTest_Moller(const Triangle & t, Vector3::Arg orig, Vector3::Arg dir, float * out_t, float * out_u, float * out_v);

	inline bool rayTest(const Triangle & t, Vector3::Arg orig, Vector3::Arg dir, float * out_t, float * out_u, float * out_v)
	{
		return rayTest_Moller(t, orig, dir, out_t, out_u, out_v);
	}
	
	inline bool overlap(const Triangle & t, const Box & b)
	{
		Vector3 center = b.center();
		Vector3 extents = b.extents();
		return triBoxOverlap(center, extents, t);
	}

	inline bool overlap(const Box & b, const Triangle & t)
	{
		return overlap(t, b);
	}

	inline bool overlapNoBounds(const Triangle & t, const Box & b)
	{
		Vector3 center = b.center();
		Vector3 extents = b.extents();
		return triBoxOverlapNoBounds(center, extents, t);
	}

} // nv namespace

#endif	// NV_MATH_TRIANGLE_H
