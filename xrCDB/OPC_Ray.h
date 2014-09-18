///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for rays.
 *	\file		IceRay.h
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICERAY_H__
#define __ICERAY_H__

	class ICEMATHS_API Ray
	{
		public:
		//! Constructor
		inline_					Ray()																{}
		//! Constructor
		inline_					Ray(const Point& orig, const Point& dir) : mOrig(orig), mDir(dir)	{}
		//! Copy constructor
		inline_					Ray(const Ray& ray) : mOrig(ray.mOrig), mDir(ray.mDir)				{}
		//! Destructor
		inline_					~Ray()																{}

						Point	mOrig;		//!< Ray origin
						Point	mDir;		//!< Normalized direction
	};

	class ICEMATHS_API Segment
	{
		public:
		//! Constructor
		inline_					Segment()															{}
		//! Constructor
		inline_					Segment(const Point& p0, const Point& p1) : mP0(p0), mP1(p1)		{}
		//! Copy constructor
		inline_					Segment(const Segment& seg) : mP0(seg.mP0), mP1(seg.mP1)			{}
		//! Destructor
		inline_					~Segment()															{}

		inline_	const	Point&	GetOrigin()			const	{ return mP0;		}
		inline_			Point	ComputeDirection()	const	{ return mP1 - mP0;	}

		inline_			void	SetOriginDirection(const Point& origin, const Point& direction)
						{
							mP0 = mP1 = origin;
							mP1 += direction;
						}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Computes a point on the segment
		 *	\param		pt	[out] point on segment
		 *	\param		t	[in] point's parameter [t=0 => pt = mP0, t=1 => pt = mP1]
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_			void	ComputePoint(Point& pt, float t)	const	{	pt = mP0 + t * (mP1 - mP0);		}

						Point	mP0;		//!< Start of segment
						Point	mP1;		//!< End of segment
	};

#endif // __ICERAY_H__
