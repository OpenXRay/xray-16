///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for planes.
 *	\file		IcePlane.h
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICEPLANE_H__
#define __ICEPLANE_H__

	#define PLANE_EPSILON		(1.0e-7f)

	class ICEMATHS_API Plane
	{
		public:
		//! Constructor
		inline_			Plane()															{													}
		//! Constructor
		inline_			Plane(float nx, float ny, float nz, float d)					{ Set(nx, ny, nz, d);								}
		//! Constructor
		inline_			Plane(const Point& p, const Point& n)							{ Set(p, n);										}
		//! Constructor
		inline_			Plane(const Point& p0, const Point& p1, const Point& p2)		{ Set(p0, p1, p2);									}
		//! Constructor
		inline_			Plane(const Point& n, float d)									{ this->n = n; this->d = d;							}
		//! Copy constructor
		inline_			Plane(const Plane& plane) : n(plane.n), d(plane.d)				{													}
		//! Destructor
		inline_			~Plane()														{													}

		inline_	Plane&	Zero()															{ n.Zero(); d = 0.0f;				return *this;	}
		inline_	Plane&	Set(float nx, float ny, float nz, float d)						{ n.Set(nx, ny, nz); this->d = d;	return *this;	}
		inline_	Plane&	Set(const Point& p, const Point& n)								{ this->n = n; d = - p | n;			return *this;	}
				Plane&	Set(const Point& p0, const Point& p1, const Point& p2);

		inline_	float	Distance(const Point& p)			const						{ return (p | n) + d;								}
		inline_	bool	Belongs(const Point& p)				const						{ return _abs(Distance(p)) < PLANE_EPSILON;		}

		inline_	void	Normalize()
		{
			float Denom = 1.0f / n.Magnitude();
			n.x	*= Denom;
			n.y	*= Denom;
			n.z	*= Denom;
			d	*= Denom;
		}

		public:
		// Members
				Point	n;		//!< The normal to the plane
				float	d;		//!< The distance from the origin

		// Cast operators
		inline_			operator Point()					const	{ return n;											}
/*		inline_			operator HPoint()					const	{ return HPoint(n, d);								}

		// Arithmetic operators
		inline_	Plane	operator*(const Matrix4x4& m)		const
						{
							// Old code from Irion. Kept for reference.
							Plane Ret(*this);
							return Ret *= m;
						}

		inline_	Plane&	operator*=(const Matrix4x4& m)
						{
							// Old code from Irion. Kept for reference.
							Point n2 = HPoint(n, 0.0f) * m;
							d = -((Point) (HPoint( -d*n, 1.0f ) * m) | n2);
							n = n2;
							return *this;
						}
*/
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	Transforms a plane by a 4x4 matrix. Same as Plane * Matrix4x4 operator, but faster.
	 *	\param		transformed	[out] transformed plane
	 *	\param		plane		[in] source plane
	 *	\param		transform	[in] transform matrix
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline_	void TransformPlane(Plane& transformed, const Plane& plane, const Matrix4x4& transform)
	{
		// Catch the rotation part of the 4x4 matrix
		Matrix3x3 Rot = transform;

		// Rotate the normal
		transformed.n = plane.n * Rot;

		// Compute _new_ d
		Point Trans;
		transform.GetTrans(Trans);
		transformed.d = (plane.d * transformed.n - Trans)|transformed.n;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	Transforms a plane by a 4x4 matrix. Same as Plane * Matrix4x4 operator, but faster.
	 *	\param		plane		[in/out] source plane (transformed on return)
	 *	\param		transform	[in] transform matrix
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline_	void TransformPlane(Plane& plane, const Matrix4x4& transform)
	{
		// Catch the rotation part of the 4x4 matrix
		Matrix3x3 Rot = transform;

		// Rotate the normal
		plane.n *= Rot;

		// Compute _new_ d
		Point Trans;
		transform.GetTrans(Trans);
		plane.d = (plane.d * plane.n - Trans)|plane.n;
	}

#endif // __ICEPLANE_H__
