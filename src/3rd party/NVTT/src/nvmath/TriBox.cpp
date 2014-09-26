/********************************************************/
/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
/* Function: int triBoxOverlap(float boxcenter[3],      */
/*          float boxhalfsize[3],float triverts[3][3]); */
/* History:                                             */
/*   2001-03-05: released the code in its first version */
/*   2001-06-18: changed the order of the tests, faster */
/*                                                      */
/* Acknowledgement: Many thanks to Pierre Terdiman for  */
/* suggestions and discussions on how to optimize code. */
/* Thanks to David Hunt for finding a ">="-bug!         */
/********************************************************/

#include <nvmath/Vector.h>
#include <nvmath/Triangle.h>

using namespace nv;

#define X 0
#define Y 1
#define Z 2

#define FINDMINMAX(x0,x1,x2,min,max) \
  min = max = x0;   \
  if(x1<min) min=x1;\
  if(x1>max) max=x1;\
  if(x2<min) min=x2;\
  if(x2>max) max=x2;


static bool planeBoxOverlap(Vector3::Arg normal, Vector3::Arg vert, Vector3::Arg maxbox)	// -NJMP-
{
	Vector3 vmin, vmax;

	float signs[3] = {1, 1, 1};
	if (normal.x() <= 0.0f) signs[0] = -1;
	if (normal.y() <= 0.0f) signs[1] = -1;
	if (normal.z() <= 0.0f) signs[2] = -1;
	
	Vector3 sign(signs[0], signs[1], signs[2]);
	vmin = -scale(sign, maxbox) - vert;
	vmax = scale(sign, maxbox) - vert;

	if (dot(normal, vmin) > 0.0f) return false;
	if (dot(normal, vmax) >= 0.0f) return true;

	return false;
}


/*======================== X-tests ========================*/
#define AXISTEST_X01(a, b, fa, fb) \
	p0 = a*v0.y() - b*v0.z(); \
	p2 = a*v2.y() - b*v2.z(); \
	if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
	rad = fa * boxhalfsize.y() + fb * boxhalfsize.z(); \
	if(min>rad || max<-rad) return false;

#define AXISTEST_X2(a, b, fa, fb) \
	p0 = a*v0.y() - b*v0.z(); \
	p1 = a*v1.y() - b*v1.z(); \
	if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
	rad = fa * boxhalfsize.y() + fb * boxhalfsize.z(); \
	if(min>rad || max<-rad) return false;

/*======================== Y-tests ========================*/
#define AXISTEST_Y02(a, b, fa, fb) \
	p0 = -a*v0.x() + b*v0.z(); \
	p2 = -a*v2.x() + b*v2.z(); \
	if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
	rad = fa * boxhalfsize.x() + fb * boxhalfsize.z(); \
	if(min>rad || max<-rad) return false;

#define AXISTEST_Y1(a, b, fa, fb) \
	p0 = -a*v0.x() + b*v0.z(); \
	p1 = -a*v1.x() + b*v1.z(); \
	if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
	rad = fa * boxhalfsize.x() + fb * boxhalfsize.z(); \
	if(min>rad || max<-rad) return false;

/*======================== Z-tests ========================*/

#define AXISTEST_Z12(a, b, fa, fb) \
	p1 = a*v1.x() - b*v1.y();	\
	p2 = a*v2.x() - b*v2.y();	\
	if(p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;} \
	rad = fa * boxhalfsize.x() + fb * boxhalfsize.y(); \
	if(min>rad || max<-rad) return false;

#define AXISTEST_Z0(a, b, fa, fb) \
	p0 = a*v0.x() - b*v0.y();	\
	p1 = a*v1.x() - b*v1.y();	\
	if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
	rad = fa * boxhalfsize.x() + fb * boxhalfsize.y(); \
	if(min>rad || max<-rad) return false;


bool triBoxOverlap(Vector3::Arg boxcenter, Vector3::Arg boxhalfsize, const Triangle & tri)
{
	// use separating axis theorem to test overlap between triangle and box
	// need to test for overlap in these directions:
	// 1) the {x,y,z}-directions (actually, since we use the AABB of the triangle
	//    we do not even need to test these)
	// 2) normal of the triangle
	// 3) crossproduct(edge from tri, {x,y,z}-directin)
	//    this gives 3x3=9 more tests
	Vector3 v0, v1, v2;
	float min, max, p0, p1, p2, rad, fex, fey, fez;
	Vector3 normal, e0, e1, e2;

	// This is the fastest branch on Sun.
	// move everything so that the boxcenter is in (0,0,0)
	v0 = tri.v[0] - boxcenter;
	v1 = tri.v[1] - boxcenter;
	v2 = tri.v[2] - boxcenter;

	// Compute triangle edges.
	e0 = v1 - v0;	// tri edge 0
	e1 = v2 - v1;	// tri edge 1
	e2 = v0 - v2;	// tri edge 2

	// Bullet 3:
	//  test the 9 tests first (this was faster)
	fex = fabsf(e0.x());
	fey = fabsf(e0.y());
	fez = fabsf(e0.z());
	AXISTEST_X01(e0.z(), e0.y(), fez, fey);
	AXISTEST_Y02(e0.z(), e0.x(), fez, fex);
	AXISTEST_Z12(e0.y(), e0.x(), fey, fex);

	fex = fabsf(e1.x());
	fey = fabsf(e1.y());
	fez = fabsf(e1.z());
	AXISTEST_X01(e1.z(), e1.y(), fez, fey);
	AXISTEST_Y02(e1.z(), e1.x(), fez, fex);
	AXISTEST_Z0(e1.y(), e1.x(), fey, fex);

	fex = fabsf(e2.x());
	fey = fabsf(e2.y());
	fez = fabsf(e2.z());
	AXISTEST_X2(e2.z(), e2.y(), fez, fey);
	AXISTEST_Y1(e2.z(), e2.x(), fez, fex);
	AXISTEST_Z12(e2.y(), e2.x(), fey, fex);

	// Bullet 1:
	//  first test overlap in the {x,y,z}-directions
	//  find min, max of the triangle each direction, and test for overlap in
	//  that direction -- this is equivalent to testing a minimal AABB around
	//  the triangle against the AABB

	// test in X-direction
	FINDMINMAX(v0.x(), v1.x(), v2.x(), min, max);
	if(min > boxhalfsize.x() || max < -boxhalfsize.x()) return false;

	// test in Y-direction
	FINDMINMAX(v0.y(), v1.y(), v2.y(), min, max);
	if(min > boxhalfsize.y() || max < -boxhalfsize.y()) return false;

	// test in Z-direction
	FINDMINMAX(v0.z(), v1.z(), v2.z(), min, max);
	if(min > boxhalfsize.z() || max < -boxhalfsize.z()) return false;

	// Bullet 2:
	//  test if the box intersects the plane of the triangle
	//  compute plane equation of triangle: normal*x+d=0
	normal = cross(e0, e1);

	return planeBoxOverlap(normal, v0, boxhalfsize);
}


bool triBoxOverlapNoBounds(Vector3::Arg boxcenter, Vector3::Arg boxhalfsize, const Triangle & tri)
{
	// use separating axis theorem to test overlap between triangle and box
	// need to test for overlap in these directions:
	// 1) the {x,y,z}-directions (actually, since we use the AABB of the triangle
	//    we do not even need to test these)
	// 2) normal of the triangle
	// 3) crossproduct(edge from tri, {x,y,z}-directin)
	//    this gives 3x3=9 more tests
	Vector3 v0, v1, v2;
	float min, max, p0, p1, p2, rad, fex, fey, fez;
	Vector3 normal, e0, e1, e2;

	// This is the fastest branch on Sun.
	// move everything so that the boxcenter is in (0,0,0)
	v0 = tri.v[0] - boxcenter;
	v1 = tri.v[1] - boxcenter;
	v2 = tri.v[2] - boxcenter;

	// Compute triangle edges.
	e0 = v1 - v0;	// tri edge 0
	e1 = v2 - v1;	// tri edge 1
	e2 = v0 - v2;	// tri edge 2

	// Bullet 3:
	//  test the 9 tests first (this was faster)
	fex = fabsf(e0.x());
	fey = fabsf(e0.y());
	fez = fabsf(e0.z());
	AXISTEST_X01(e0.z(), e0.y(), fez, fey);
	AXISTEST_Y02(e0.z(), e0.x(), fez, fex);
	AXISTEST_Z12(e0.y(), e0.x(), fey, fex);

	fex = fabsf(e1.x());
	fey = fabsf(e1.y());
	fez = fabsf(e1.z());
	AXISTEST_X01(e1.z(), e1.y(), fez, fey);
	AXISTEST_Y02(e1.z(), e1.x(), fez, fex);
	AXISTEST_Z0(e1.y(), e1.x(), fey, fex);

	fex = fabsf(e2.x());
	fey = fabsf(e2.y());
	fez = fabsf(e2.z());
	AXISTEST_X2(e2.z(), e2.y(), fez, fey);
	AXISTEST_Y1(e2.z(), e2.x(), fez, fex);
	AXISTEST_Z12(e2.y(), e2.x(), fey, fex);

	// Bullet 2:
	//  test if the box intersects the plane of the triangle
	//  compute plane equation of triangle: normal*x+d=0
	normal = cross(e0, e1);

	return planeBoxOverlap(normal, v0, boxhalfsize);
}
