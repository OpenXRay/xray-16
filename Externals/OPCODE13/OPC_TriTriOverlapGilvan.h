///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This is an optional header that implements a custom triangle-versus triangle intersection test.
// This test can be used .
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define FindMinAndMax(m,M)  \
	if ( dp1 < m )          \
        m = dp1;            \
	else					\
        if ( dp1 > M )      \
            M = dp1;        \
                            \
    if ( dp2 < m )          \
        m = dp2;            \
	else					\
		if ( dp2 > M )		\
			M = dp2;

#define TriProjection( rkD, a0,a1,a2, mi, ma )	\
{												\
	mi = ma= rkD|a0;							\
	const float dp1 = rkD|a1;					\
    const float dp2 = rkD|a2;					\
												\
	FindMinAndMax(mi,ma)						\
}

#define TriProjection2D( a0,a1,a2, mi, ma )     \
{												\
    mi = ma = (ni0*a0[i0] + ni1*a0[i1]);		\
    const float dp1 = ni0*a1[i0] + ni1*a1[i1];  \
    const float dp2 = ni0*a2[i0] + ni1*a2[i1];  \
                                                \
	FindMinAndMax(mi,ma)						\
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Triangle/triangle intersection test routine, by Gilvan Maia, 2004.
 *      Some little optimizations were done and added a 2D SAT test for coplanar triangles.
 *      This test is based on the tri-tri test from http://www.magik-software.com
 *
 *	\param		V0		[in] triangle 0, vertex 0
 *	\param		V1		[in] triangle 0, vertex 1
 *	\param		V2		[in] triangle 0, vertex 2
 *	\param		U0		[in] triangle 1, vertex 0
 *	\param		U1		[in] triangle 1, vertex 1
 *	\param		U2		[in] triangle 1, vertex 2
 *	\return		true if triangles overlap
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline_ BOOL AABBTreeCollider::TriTriOverlap(const Point& u0, const Point& u1, const Point& u2, const Point& v0, const Point& v1, const Point& v2)
{
	// Stats
	mNbPrimPrimTests++;

    float fVMin, fVMax;

    // direction N
    const Point e0 = u1-u0;
    const Point e1 = u2-u1;
    const Point kN = e0 ^ e1;
    const float fNdU0 = kN | u0;//u0u1u02 origin
    TriProjection(kN,v0,v1,v2,fVMin,fVMax);

    if ( fVMin > fNdU0 || fNdU0 > fVMax )
        return FALSE;

    // direction M
    const Point f0 = v1-v0;
    const Point f1 = v2-v1;
    const Point kM = f0 ^ f1;

    if ( (kN ^ kM).SquareMagnitude() >= 1.0e-6f )
    {
        // triangles are NOT coplanar... is this numerically stable/precise?
        const float fMdV0 = kM | v0;
        TriProjection(kM,u0,u1,u2,fVMin,fVMax);
        if ( fMdV0 < fVMin || fMdV0 > fVMax )
            return FALSE;

        // Sweet macro that makes code shorter and understandable
        #define TEST_AXES( E, F )						\
		{												\
			const Point kDir = E ^ F;					\
			float fUMin, fUMax;							\
			TriProjection(kDir,u0,u1,u2,fUMin,fUMax);	\
			TriProjection(kDir,v0,v1,v2,fVMin,fVMax);	\
			if ( fUMax < fVMin || fVMax < fUMin )		\
				return FALSE;							\
		}

        // computes remaining edges
        // directions E[i0]xF[i1]
		//float fUMin, fUMax;

        TEST_AXES( e0, f0 )
		TEST_AXES( e0, f1 )
        TEST_AXES( e1, f1 )
		TEST_AXES( e1, f0 )

		const Point e2 = u0-u2;
		TEST_AXES( e2, f0 )
		TEST_AXES( e2, f1 )

		const Point f2 = v0-v2;
        TEST_AXES( e2, f2 )
        TEST_AXES( e1, f2 )
        TEST_AXES( e0, f2 )

		return TRUE;
    }
    else
    {
        // NOTE: If they're coplanar, then their normals are the same??
#define SATUSE_3D_AXES
#ifdef SATUSE_3D_AXES

        // this code looks like the original one, but with few changes
		float fUMin, fUMax;
        TEST_AXES( kN, e0 )
        TEST_AXES( kN, e1 )
        TEST_AXES( kM, f0 )
        TEST_AXES( kM, f1 )

		const Point e2 = u0-u2;
		TEST_AXES( kN, e2 )
		const Point f2 = u0-u2;
        TEST_AXES( kM, f2 )
#else
        // brand new code
		const Point e2 = u0-u2;
        const Point f2 = v0-v2;
		const float A[3] = { fabsf(kN[0]), fabsf(kN[1]), fabsf(kN[2])};
        float ni0, ni1;
        short i0,i1;

		// project onto an axis-aligned plane, that maximizes the area
        if(A[0]>A[1])
        {
            if(A[0]>A[2])
            {
                i0=1;      /// A[0] is greatest
                i1=2;
            }
            else
            {
                i0=0;      // A[2] is greatest
                i1=1;
            }
        }
        else   // A[0]<=A[1]
        {
            if(A[2]>A[1])
            {
                i0=0;      // A[2] is greatest
                i1=1;
            }
            else
            {
                i0=0;      // A[1] is greatest
                i1=2;
            }
        }

        #define TEST_AXES2D(edge)					  \
			ni0 =  edge[i0];                          \
			ni1 = -edge[i1];                          \
			TriProjection2D(u0,u1,u2,fUMin,fUMax);    \
			TriProjection2D(v0,v1,v2,fVMin,fVMax);    \
			if ( fUMax < fVMin || fVMax < fUMin )     \
				return false;

		float fUMin, fUMax;
        TEST_AXES2D( e0 )
        TEST_AXES2D( e1 )
        TEST_AXES2D( e2 )

        TEST_AXES2D( f0 )
        TEST_AXES2D( f1 )
        TEST_AXES2D( f2 )
#endif

		return TRUE;
    }
}
