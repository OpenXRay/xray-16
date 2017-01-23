#define LOCAL_EPSILON 0.000001f

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Computes a ray-triangle intersection test.
 *	From Tomas Möller's "Fast Minimum Storage Ray-Triangle Intersection"
 *
 *	\param		vert0	[in] triangle vertex
 *	\param		vert1	[in] triangle vertex
 *	\param		vert2	[in] triangle vertex
 *	\param		t		[out] distance
 *	\param		u		[out] impact barycentric coordinate
 *	\param		v		[out] impact barycentric coordinate
 *	\return		true on overlap
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline_ BOOL RayCollider::RayTriOverlap(const Point& vert0, const Point& vert1, const Point& vert2)
{
	// Stats
	mNbRayPrimTests++;

	// Find vectors for two edges sharing vert0
	Point edge1 = vert1 - vert0;
	Point edge2 = vert2 - vert0;

	// Begin calculating determinant - also used to calculate U parameter
	Point pvec = mDir^edge2;

	// If determinant is near zero, ray lies in plane of triangle
	float det = edge1|pvec;

	if(mCulling)
	{
		if(det<LOCAL_EPSILON)														return FALSE;
		// From here, det is > 0. So we can use integer cmp.

		// Calculate distance from vert0 to ray origin
		Point tvec = mOrigin - vert0;

		// Calculate U parameter and test bounds
		mStabbedFace.mU = tvec|pvec;
//		if(IR(u)&0x80000000 || u>det)					return FALSE;
		if(IR(mStabbedFace.mU)&0x80000000 || IR(mStabbedFace.mU)>IR(det))			return FALSE;

		// Prepare to test V parameter
		Point qvec = tvec^edge1;

		// Calculate V parameter and test bounds
		mStabbedFace.mV = mDir|qvec;
		if(IR(mStabbedFace.mV)&0x80000000 || mStabbedFace.mU+mStabbedFace.mV>det)	return FALSE;

		// Calculate t, scale parameters, ray intersects triangle
		mStabbedFace.mDistance = edge2|qvec;
		float inv_det = 1.0f / det;
		mStabbedFace.mDistance *= inv_det;
		mStabbedFace.mU *= inv_det;
		mStabbedFace.mV *= inv_det;
	}
	else
	{
		// the non-culling branch
		if(det>-LOCAL_EPSILON && det<LOCAL_EPSILON)									return FALSE;
		float inv_det = 1.0f / det;

		// Calculate distance from vert0 to ray origin
		Point tvec = mOrigin - vert0;

		// Calculate U parameter and test bounds
		mStabbedFace.mU = (tvec|pvec) * inv_det;
//		if(IR(u)&0x80000000 || u>1.0f)					return FALSE;
		if(IR(mStabbedFace.mU)&0x80000000 || IR(mStabbedFace.mU)>IEEE_1_0)			return FALSE;

		// prepare to test V parameter
		Point qvec = tvec^edge1;

		// Calculate V parameter and test bounds
		mStabbedFace.mV = (mDir|qvec) * inv_det;
		if(IR(mStabbedFace.mV)&0x80000000 || mStabbedFace.mU+mStabbedFace.mV>1.0f)	return FALSE;

		// Calculate t, ray intersects triangle
		mStabbedFace.mDistance = (edge2|qvec) * inv_det;
	}
	return TRUE;
}
