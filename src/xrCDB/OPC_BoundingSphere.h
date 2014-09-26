///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code to compute the minimal bounding sphere.
 *	\file		IceBoundingSphere.h
 *	\author		Pierre Terdiman
 *	\date		January, 29, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICEBOUNDINGSPHERE_H__
#define __ICEBOUNDINGSPHERE_H__

	class MESHMERIZER_API Sphere
	{
		public:
		//! Constructor
		inline_					Sphere()																		{}
		//! Constructor
		inline_					Sphere(const Point& center, float radius) : mCenter(center), mRadius(radius)	{}
		//! Constructor
								Sphere(const udword n, Point* p);
		//! Copy constructor
		inline_					Sphere(const Sphere& sphere) : mCenter(sphere.mCenter), mRadius(sphere.mRadius)	{}
		//! Destructor
		inline_					~Sphere()																		{}

				bool			Compute(udword nbverts, Point* verts);
				bool			FastCompute(udword nbverts, Point* verts);

		// Access methods
		inline_	const Point&	GetCenter()		const						{ return mCenter; }
		inline_	float			GetRadius()		const						{ return mRadius; }

		inline_	const Point&	Center()		const						{ return mCenter; }
		inline_	float			Radius()		const						{ return mRadius; }

		inline_	Sphere&			Set(const Point& center, float radius)		{ mCenter = center; mRadius = radius; return *this; }
		inline_	Sphere&			SetCenter(const Point& center)				{ mCenter = center; return *this; }
		inline_	Sphere&			SetRadius(float radius)						{ mRadius = radius; return *this; }

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Tests if a point is contained within the sphere.
		 *	\param		p	[in] the point to test
		 *	\return		true if inside the sphere
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_	bool			Contains(const Point& p)	const
								{
									return mCenter.SquareDistance(p) < mRadius*mRadius;
								}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Tests if a sphere is contained within the sphere.
		 *	\param		sphere	[in] the sphere to test
		 *	\return		true if inside the sphere
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_	bool			Contains(const Sphere& sphere)	const
								{
									float r = mRadius - sphere.mRadius;
									return mCenter.SquareDistance(sphere.mCenter) < r*r;
								}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Tests if a box is contained within the sphere.
		 *	\param		aabb	[in] the box to test
		 *	\return		true if inside the sphere
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_ BOOL			Contains(const AABB& aabb)	const
								{
									// I assume if all 8 box vertices are inside the sphere, so does the whole box.
									// Sounds ok but maybe there's a better way?
									float R2 = mRadius * mRadius;
									Point Max; aabb.GetMax(Max);
									Point Min; aabb.GetMin(Min);

									Point p;
									p.x=Max.x; p.y=Max.y; p.z=Max.z;	if(mCenter.SquareDistance(p)>=R2)	return FALSE;
									p.x=Min.x;							if(mCenter.SquareDistance(p)>=R2)	return FALSE;
									p.x=Max.x; p.y=Min.y;				if(mCenter.SquareDistance(p)>=R2)	return FALSE;
									p.x=Min.x;							if(mCenter.SquareDistance(p)>=R2)	return FALSE;
									p.x=Max.x; p.y=Max.y; p.z=Min.z;	if(mCenter.SquareDistance(p)>=R2)	return FALSE;
									p.x=Min.x;							if(mCenter.SquareDistance(p)>=R2)	return FALSE;
									p.x=Max.x; p.y=Min.y;				if(mCenter.SquareDistance(p)>=R2)	return FALSE;
									p.x=Min.x;							if(mCenter.SquareDistance(p)>=R2)	return FALSE;

									return TRUE;
								}

		public:
				Point			mCenter;		//!< Sphere center
				float			mRadius;		//!< Sphere radius
#ifdef OLD
		private:
				Sphere			PlanarCircumscribe3							(const Point& p0, const Point& p1, const Point& p2);
				Sphere			Circumscribe4								(const Point& p0, const Point& p1, const Point& p2, const Point& p3);
				Sphere			MinFix3										(int n, Point** perm, Point* fixed0, Point* fixed1, Point* fixed2);
				Sphere			MinFix2										(int n, Point** perm, Point* fixed0, Point* fixed1);
				Sphere			MinFix1										(int n, Point** perm, Point* fixed0);

				int				PointInsideSphere							(const Point& p, const Sphere& s);
				Sphere			MinimalSphere1								(const Point& p);
				Sphere			MinimalSphere2								(const Point& p0, const Point& p1);
				Sphere			MinimalSphere3								(const Point& p0, const Point& p1, const Point& p2);
				Sphere			MinimalSphere4								(const Point& p0, const Point& p1, const Point& p2, const Point& p3);
#endif
	};

#endif // __ICEBOUNDINGSPHERE_H__
