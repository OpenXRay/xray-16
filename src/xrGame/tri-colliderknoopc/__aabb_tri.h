#pragma  once

	class  Point
	{
		public:

		//! Empty constructor
		IC				Point()														{}
		//! Constructor from floats
		IC				Point(float _x, float _y, float _z) : x(_x), y(_y), z(_z)	{}
		//! Constructor from array
		IC				Point(float f[3]) : x(f[0]), y(f[1]), z(f[2])				{}
		//! Constructor from array
		IC				Point(const float f[3]) : x(f[0]), y(f[1]), z(f[2])			{}
		//! Copy constructor
		IC				Point(const Point& p) : x(p.x), y(p.y), z(p.z)				{}
		//! Destructor
		IC				~Point()													{}

		//! Returns MIN(x, y, z);
		//IC	float		Min()								const		{ return MIN(x, MIN(y, z));												}
		//! Returns MAX(x, y, z);
		//IC	float		Max()								const		{ return MAX(x, MAX(y, z));												}
		//! TO BE DOCUMENTED
		//IC	Point&		Min(const Point& p)								{ x = MIN(x, p.x); y = MIN(y, p.y); z = MIN(z, p.z);	return *this;	}
		//! TO BE DOCUMENTED
		//IC	Point&		Max(const Point& p)								{ x = MAX(x, p.x); y = MAX(y, p.y); z = MAX(z, p.z);	return *this;	}

		//! Computes square magnitude
		IC	float		SquareMagnitude()					const		{ return x*x + y*y + z*z;												}
		//! Computes magnitude
		IC	float		Magnitude()							const		{ return _sqrt(x*x + y*y + z*z);										}

		//! Return largest axis
		IC	u32			LargestAxis()						const
						{
							const float* Vals = &x;
							u32 m = 0;
							if(Vals[1] > Vals[m]) m = 1;
							if(Vals[2] > Vals[m]) m = 2;
							return m;
						}

		// Arithmetic operators
		//! Operator for Point Negate = - Point
		ICF	Point		operator-()							const		{ return Point(-x, -y, -z);							}

		//! Operator for Point Plus = Point + Point.
		ICF	Point		operator+(const Point& p)			const		{ return Point(x + p.x, y + p.y, z + p.z);			}
		//! Operator for Point Minus = Point - Point.
		ICF	Point		operator-(const Point& p)			const		{ return Point(x - p.x, y - p.y, z - p.z);			}
		//! Operator for Point Scale = Point * float.
		ICF	Point		operator*(float s)					const		{ return Point(x * s,   y * s,   z * s );			}
		//! Operator for Point Scale = float * Point.
		friend			Point		operator*(float s, const Point& p)				{ return Point(s * p.x, s * p.y, s * p.z);			}
		//! Operator for Point Scale = Point / float.
		ICF	Point		operator/(float s)					const		{ s = 1.0f / s; return Point(x * s, y * s, z * s);	}

		//! Operator for float DotProd = Point | Point.
		ICF	float		operator|(const Point& p)			const		{ return x*p.x + y*p.y + z*p.z;						}
		//! Operator for Point VecProd = Point ^ Point.
		ICF	Point		operator^(const Point& p)			const
						{
							return Point(
							y * p.z - z * p.y,
							z * p.x - x * p.z,
							x * p.y - y * p.x );
						}

		//! Operator for Point += Point.
		ICF	Point&		operator+=(const Point& p)						{ x += p.x; y += p.y; z += p.z;	return *this;		}
		//! Operator for Point += float.
		ICF	Point&		operator+=(float s)								{ x += s;   y += s;   z += s;	return *this;		}

		//! Operator for Point -= Point.
		ICF	Point&		operator-=(const Point& p)						{ x -= p.x; y -= p.y; z -= p.z;	return *this;		}
		//! Operator for Point -= float.
		ICF	Point&		operator-=(float s)								{ x -= s;   y -= s;   z -= s;	return *this;		}
		//! Operator for Point *= float.
		ICF	Point&		operator*=(float s)								{ x *= s; y *= s; z *= s;		return *this;		}
		//! Operator for Point /= float.
		ICF	Point&		operator/=(float s)								{ s = 1.0f/s; x *= s; y *= s; z *= s; return *this; }

		// Arithmetic operators
		//! Operator for Point Mul = Point * Matrix3x3.
	//					Point		operator*(const Matrix3x3& mat)		const;
		//! Operator for Point Mul = Point * Matrix4x4.
	//					Point		operator*(const Matrix4x4& mat)		const;
		//! Operator for Point *= Matrix3x3.
	///					Point&		operator*=(const Matrix3x3& mat);
		//! Operator for Point *= Matrix4x4.
	//					Point&		operator*=(const Matrix4x4& mat);

		//! Access as array
		ICF				operator	const	float*() const	{ return &x; }
		//! Access as array
		ICF				operator			float*()		{ return &x; }

		public:
						float		x;		//!< x coordinate
						float		y;		//!< y coordinate
						float		z;		//!< z coordinate
	};
//using namespace CDB;
using namespace Opcode;
//typedef float*	Point;	
//typedef	dVector3 Point
//! This macro quickly finds the min & max values among 3 variables
#define FINDMINMAX(x0, x1, x2, min, max)	\
	min = max = x0;							\
	if(x1<min) min=x1;						\
	if(x1>max) max=x1;						\
	if(x2<min) min=x2;						\
	if(x2>max) max=x2;

#define EXITMINMAX(x0, x1, x2, min, max)		\
	if(x0<min&&x1<min&&x2<min)return false;		\
	if(x0>max&&x1>max&&x2>max)return false;

//! TO BE DOCUMENTED
ICF bool planeBoxOverlap_slow(const Point& normal, const float d, const Point& maxbox)
{
	Point vmin, vmax;
	if(normal[0]>0.0f)	{ vmin[0] =-maxbox[0]; vmax[0]= maxbox[0]; }
	else				{ vmin[0] = maxbox[0]; vmax[0]=-maxbox[0]; }
	if(normal[1]>0.0f)	{ vmin[1] =-maxbox[1]; vmax[1]= maxbox[1]; }
	else				{ vmin[1] = maxbox[1]; vmax[1]=-maxbox[1]; }
	if(normal[2]>0.0f)	{ vmin[2] =-maxbox[2]; vmax[2]= maxbox[2]; }
	else				{ vmin[2] = maxbox[2]; vmax[2]=-maxbox[2]; }
	if((normal|vmin)+d >= 0.0f)	return false;
	if((normal|vmax)+d >  0.0f)	return true;
	return	false		;
}

ICF bool planeBoxOverlap(const Point& normal, const float d, const Point& maxbox)
{
	float norm_box_proj=_abs(maxbox.x*normal.x)+_abs(maxbox.y*normal.y)+_abs(maxbox.z*normal.z);
	if(d>-norm_box_proj&&d<norm_box_proj)return true;
	else return false;
}

//! TO BE DOCUMENTED
#define AXISTEST_X01(a, b, fa, fb)							\
	min = a*v0.y - b*v0.z;									\
	max = a*v2.y - b*v2.z;									\
	if(min>max) {const float tmp=max; max=min; min=tmp;	}	\
	rad = fa * extents.y + fb * extents.z;					\
	if(min>rad || max<-rad) return false;

//! TO BE DOCUMENTED
#define AXISTEST_X2(a, b, fa, fb)							\
	min = a*v0.y - b*v0.z;									\
	max = a*v1.y - b*v1.z;									\
	if(min>max) {const float tmp=max; max=min; min=tmp;	}	\
	rad = fa * extents.y + fb * extents.z;					\
	if(min>rad || max<-rad) return false;

//! TO BE DOCUMENTED
#define AXISTEST_Y02(a, b, fa, fb)							\
	min = b*v0.z - a*v0.x;									\
	max = b*v2.z - a*v2.x;									\
	if(min>max) {const float tmp=max; max=min; min=tmp;	}	\
	rad = fa * extents.x + fb * extents.z;					\
	if(min>rad || max<-rad) return false;

//! TO BE DOCUMENTED
#define AXISTEST_Y1(a, b, fa, fb)							\
	min = b*v0.z - a*v0.x;									\
	max = b*v1.z - a*v1.x;									\
	if(min>max) {const float tmp=max; max=min; min=tmp;	}	\
	rad = fa * extents.x + fb * extents.z;					\
	if(min>rad || max<-rad) return false;

//! TO BE DOCUMENTED
#define AXISTEST_Z12(a, b, fa, fb)							\
	min = a*v1.x - b*v1.y;									\
	max = a*v2.x - b*v2.y;									\
	if(min>max) {const float tmp=max; max=min; min=tmp;	}	\
	rad = fa * extents.x + fb * extents.y;					\
	if(min>rad || max<-rad) return false;

//! TO BE DOCUMENTED
#define AXISTEST_Z0(a, b, fa, fb)							\
	min = a*v0.x - b*v0.y;									\
	max = a*v1.x - b*v1.y;									\
	if(min>max) {const float tmp=max; max=min; min=tmp;	}	\
	rad = fa * extents.x + fb * extents.y;					\
	if(min>rad || max<-rad) return false;
	


IC	bool		aabb_tri_aabb(Point center,Point extents,const Point* mLeafVerts)
{
	Point v0, v1, v2;
	//Fvector v0,v1,v2;
	v0.x = mLeafVerts[0].x - center.x;
	v1.x = mLeafVerts[1].x - center.x;
	v2.x = mLeafVerts[2].x - center.x;

	// First, test overlap in the {x,y,z}-directions
	//float min,max;
	// Find min, max of the triangle in x-direction, and test for overlap in X
	//FINDMINMAX(v0.x, v1.x, v2.x, min, max);
	//if(min>extents.x || max<-extents.x) return false;
	EXITMINMAX(v0.x, v1.x, v2.x,-extents.x,extents.x)
	// Same for Y
	v0.y = mLeafVerts[0].y - center.y;
	v1.y = mLeafVerts[1].y - center.y;
	v2.y = mLeafVerts[2].y - center.y;

	//FINDMINMAX(v0.y, v1.y, v2.y, min, max);
	//if(min>extents.y || max<-extents.y) return false;
	EXITMINMAX(v0.y, v1.y, v2.y,-extents.y,extents.y)
	// Same for Z
	v0.z = mLeafVerts[0].z - center.z;
	v1.z = mLeafVerts[1].z - center.z;
	v2.z = mLeafVerts[2].z - center.z;

	//FINDMINMAX(v0.z, v1.z, v2.z, min, max);
	//if(min>extents.z || max<-extents.z) return false;
	EXITMINMAX(v0.z, v1.z, v2.z,-extents.z,extents.z)
	// 2) Test if the box intersects the plane of the triangle
	// compute plane equation of triangle: normal*x+d=0
	// ### could be precomputed since we use the same leaf triangle several times
	const Point e0 = v1 - v0;
	const Point e1 = v2 - v1;
	const Point normal = e0 ^ e1;
	const float d = -normal|v0;
	bool	r0 = planeBoxOverlap(normal, d, extents);
	
#ifdef DEBUG
	if	(r0 != planeBoxOverlap_slow(normal, d, extents))
	{
		Msg("planeBoxOverlap != planeBoxOverlap_slow");
		Msg("normal %f,%f,%f", normal.x,normal.y,normal.z);
		Msg("dist %f",d);
		Msg("extents %f,%f,%f", extents.x,extents.y,extents.z);
	}
#endif
	if(!r0)		return false;
	return true;
}
IC	bool		__aabb_tri		(Point center,Point extents,const Point* mLeafVerts)
	{
		// move everything so that the boxcenter is in (0,0,0) 
		Point v0, v1, v2;
		//Fvector v0,v1,v2;
		v0.x = mLeafVerts[0].x - center.x;
		v1.x = mLeafVerts[1].x - center.x;
		v2.x = mLeafVerts[2].x - center.x;
		
		// First, test overlap in the {x,y,z}-directions
		float min,max;
		// Find min, max of the triangle in x-direction, and test for overlap in X
		FINDMINMAX(v0.x, v1.x, v2.x, min, max);
		if(min>extents.x || max<-extents.x) return false;
		
		// Same for Y
		v0.y = mLeafVerts[0].y - center.y;
		v1.y = mLeafVerts[1].y - center.y;
		v2.y = mLeafVerts[2].y - center.y;
		
		FINDMINMAX(v0.y, v1.y, v2.y, min, max);
		if(min>extents.y || max<-extents.y) return false;
		
		// Same for Z
		v0.z = mLeafVerts[0].z - center.z;
		v1.z = mLeafVerts[1].z - center.z;
		v2.z = mLeafVerts[2].z - center.z;
		
		FINDMINMAX(v0.z, v1.z, v2.z, min, max);
		if(min>extents.z || max<-extents.z) return false;
		
		// 2) Test if the box intersects the plane of the triangle
		// compute plane equation of triangle: normal*x+d=0
		// ### could be precomputed since we use the same leaf triangle several times
		const Point e0 = v1 - v0;
		const Point e1 = v2 - v1;
		const Point normal = e0 ^ e1;
		const float d = -normal|v0;
		bool	r0 = planeBoxOverlap(normal, d, extents);
#ifdef DEBUG
		if	(r0 != planeBoxOverlap_slow(normal, d, extents))
		{
			Msg("planeBoxOverlap != planeBoxOverlap_slow");
			Msg("normal %f,%f,%f", normal.x,normal.y,normal.z);
			Msg("dist %f",d);
			Msg("extents %f,%f,%f", extents.x,extents.y,extents.z);
		}
#endif
		if		(!r0)	return false;
		
		// 3) "Class III" tests
		//if(bClass3)
		{
			float rad;
			float min, max;
			// compute triangle edges
			// - edges lazy evaluated to take advantage of early exits
			// - fabs precomputed (half less work, possible since extents are always >0)
			// - customized macros to take advantage of the null component
			// - axis vector3 discarded, possibly saves useless movs
			
			const float fey0 = _abs(e0.y);
			const float fez0 = _abs(e0.z);
			AXISTEST_X01(e0.z, e0.y, fez0, fey0);
			const float fex0 = _abs(e0.x);
			AXISTEST_Y02(e0.z, e0.x, fez0, fex0);
			AXISTEST_Z12(e0.y, e0.x, fey0, fex0);
			
			const float fey1 = _abs(e1.y);
			const float fez1 = _abs(e1.z);
			AXISTEST_X01(e1.z, e1.y, fez1, fey1);
			const float fex1 = _abs(e1.x);
			AXISTEST_Y02(e1.z, e1.x, fez1, fex1);
			AXISTEST_Z0(e1.y, e1.x, fey1, fex1);
			
			const Point e2 = mLeafVerts[0] - mLeafVerts[2];
			const float fey2 = _abs(e2.y);
			const float fez2 = _abs(e2.z);
			AXISTEST_X2(e2.z, e2.y, fez2, fey2);
			const float fex2 = _abs(e2.x);
			AXISTEST_Y1(e2.z, e2.x, fez2, fex2);
			AXISTEST_Z12(e2.y, e2.x, fey2, fex2);
		}
		return true;
	}
	

