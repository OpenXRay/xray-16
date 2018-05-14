// This code is in the public domain -- Ignacio Castaño <castanyo@yahoo.es>

#include <nvmath/Triangle.h>

using namespace nv;


/// Tomas Möller, barycentric ray-triangle test.
bool rayTest_Moller(const Triangle & t, Vector3::Arg orig, Vector3::Arg dir, float * out_t, float * out_u, float * out_v)
{
	// find vectors for two edges sharing vert0 
	Vector3 e1 = t.v[1] - t.v[0];
	Vector3 e2 = t.v[2] - t.v[0];

	// begin calculating determinant - also used to calculate U parameter
	Vector3 pvec = cross(dir, e2);
	
	// if determinant is near zero, ray lies in plane of triangle
	float det = dot(e1, pvec);
	if (det < -NV_EPSILON) {
		return false;
	}

	// calculate distance from vert0 to ray origin
	Vector3 tvec = orig - t.v[0];

	// calculate U parameter and test bounds
	float u = dot(tvec, pvec);
	if( u < 0.0f || u > det ) {
		return false;
	}

	// prepare to test V parameter
	Vector3 qvec = cross(tvec, e1);

	// calculate V parameter and test bounds
	float v = dot(dir, qvec);
	if (v < 0.0f || u + v > det) {
		return false;
	}

	// calculate t, scale parameters, ray intersects triangle
	float inv_det = 1.0f / det;
	*out_t = dot(e2, qvec) * inv_det;
	*out_u = u * inv_det;	// v
	*out_v = v * inv_det;	// 1-(u+v)

	return true;
}





#if 0


// IC: This code is adapted from my Pi.MathLib code, based on Moller-Trumbore triangle test.
FXVector3 edge1, edge2, pvec, tvec, qvec;

edge1 = tri.V1 - tri.V0;
edge2 = tri.V2 - tri.V0;

pvec.Cross(ray.Direction, edge2);

float det = FXVector3.Dot(edge1, pvec);

// calculate distance from vert0 to ray origin.
FXVector3 tvec = ray.Origin - vert0;

if( det < 0 ) 
{
	// calculate U parameter and test bounds.
	float u = FXVector3.Dot(tvec, pvec);
	if (u > 0.0 || u < det)
	{
		return false;
	}

	// prepare to test V parameter.
	qvec.Cross(tvec, edge1);

	// calculate V parameter and test bounds.
	float v = FXVector3.Dot(dir, qvec);

	return v <= 0.0 && u + v >= det;
}
else
{
	// calculate U parameter and test bounds.
	float u = FXVector3.Dot(tvec, pvec);
	if (u < 0.0 || u > det)
	{
		return false;
	}

	// prepare to test V parameter.
	qvec.Cross(tvec, edge1);

	// calculate V parameter and test bounds.
	float v = FXVector3.Dot(dir, qvec);

	return v >= 0.0 && u + v <= det;
}



/** 
 * Dan Sunday, parametric ray-triangle test.
 */
//    Output: *I = intersection point (when it exists)
//    Return: -1 = triangle is degenerate (a segment or point)
//             0 = disjoint (no intersect)
//             1 = intersect in unique point I1
//             2 = are in the same plane
bool RayTriangleTest( const Vec3 &p0, const Vec3 &p1, 
					  const Vec3 &v0, const Vec3 &v1, const Vec3 &v2, const Vec3 &n,
					  Vec3 &I ) {
    Vec3 u, v;					// triangle vectors
    Vec3 dir, w0, w;			// ray vectors
    float r, a, b;				// params to calc ray-plane intersect

    // get triangle edge vectors and plane normal
    u.Sub( v1, v0 );
    v.Sub( v2, v0 );

    dir.Sub( p1, p0 );			// ray direction vector
	w0.Sub( p0, v0 );
    a = Vec3DotProduct( n, w0 );
    b = Vec3DotProduct( n, dir );

    if( fabs(b) < TI_EPSILON ) 	// ray is parallel to triangle plane
		return false;


    // get intersect point of ray with triangle plane
    r = -a / b;
    if( r < 0.0f )				// ray goes away from triangle
        return false;			// => no intersect
    
	// for a segment, also test if (r > 1.0) => no intersect

	I.Mad( p0, dir, r );		// intersect point of ray and plane

    // is I inside T?
    float    uu, uv, vv, wu, wv, D;
    uu = Vec3DotProduct( u, u );
    uv = Vec3DotProduct( u, v );
    vv = Vec3DotProduct( v, v );
    w = I - v0;
    wu = Vec3DotProduct( w, u );
    wv = Vec3DotProduct( w, v );
    D = uv * uv - uu * vv;

    // get and test parametric coords
    float s, t;
    s = (uv * wv - vv * wu) / D;
    if( s<0.0 || s > 1.0)        // I is outside T
        return false;
    t = (uv * wu - uu * wv) / D;
    if( t<0.0 || (s + t) > 1.0)  // I is outside T
        return false;

    return true;                      // I is in T
}


#endif // 0
