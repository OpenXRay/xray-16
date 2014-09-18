//---------------------------------------------------------------------------
#ifndef intersectH
#define intersectH

namespace CDB
{
	//----------------------------------------------------------------------
	// Name  : intersectRaySphere()
	// Input : rO - origin of ray in world space
	//         rV - _vector_ describing direction of ray in world space
	//         sO - Origin of sphere
	//         sR - radius of sphere
	// Notes : Normalized directional vectors expected
	// -----------------------------------------------------------------------
	IC bool IntersectRaySphere(const Fvector& rO, const Fvector& rV, const Fvector& sO, float sR)
	{
		Fvector Q;
		Q.sub(sO,rO);

		float c = Q.magnitude();
		float v = Q.dotproduct(rV);
		float d = sR*sR - (c*c - v*v);

		// If there was no intersection, return -1
		return (d > 0.0);
	}

	//-- Ray-Triangle : 2nd level of indirection --------------------------------
	IC bool TestRayTri(const Fvector& C, const Fvector& D, Fvector** p, float& u, float& v, float& range, bool bCull)
	{
		Fvector edge1, edge2, tvec, pvec, qvec;
		float det,inv_det;
		// find vectors for two edges sharing vert0
		edge1.sub(*p[1], *p[0]);
		edge2.sub(*p[2], *p[0]);
		// begin calculating determinant - also used to calculate U parameter
		pvec.crossproduct(D, edge2);
		// if determinant is near zero, ray lies in plane of triangle
		det = edge1.dotproduct(pvec);
		if (bCull){						// define TEST_CULL if culling is desired
			if (det < EPS)  return false;
			tvec.sub(C, *p[0]);							// calculate distance from vert0 to ray origin
			u = tvec.dotproduct(pvec);			// calculate U parameter and test bounds
			if (u < 0.0 || u > det) return false;
			qvec.crossproduct(tvec, edge1);				// prepare to test V parameter
			v = D.dotproduct(qvec);			// calculate V parameter and test bounds
			if (v < 0.0 || u + v > det) return false;
			range = edge2.dotproduct(qvec);		// calculate t, scale parameters, ray intersects triangle
			inv_det = 1.0f / det;
			range *= inv_det;
			u *= inv_det;
			v *= inv_det;
		}else{											// the non-culling branch
			if (det > -EPS && det < EPS) return false;
			inv_det = 1.0f / det;
			tvec.sub(C, *p[0]);							// calculate distance from vert0 to ray origin
			u = tvec.dotproduct(pvec)*inv_det;	// calculate U parameter and test bounds
			if (u < 0.0f || u > 1.0f)    return false;
			qvec.crossproduct(tvec, edge1);				// prepare to test V parameter
			v = D.dotproduct(qvec)*inv_det;	// calculate V parameter and test bounds
			if (v < 0.0f || u + v > 1.0f) return false;
			range = edge2.dotproduct(qvec)*inv_det;// calculate t, ray intersects triangle
		}
		return true;
	}
	//-- Ray-Triangle : 1st level of indirection --------------------------------
	IC bool TestRayTri(const Fvector& C, const Fvector& D, Fvector* p, float& u, float& v, float& range, bool bCull)
	{
		Fvector edge1, edge2, tvec, pvec, qvec;
		float det,inv_det;
		// find vectors for two edges sharing vert0
		edge1.sub(p[1], p[0]);
		edge2.sub(p[2], p[0]);
		// begin calculating determinant - also used to calculate U parameter
		pvec.crossproduct(D, edge2);
		// if determinant is near zero, ray lies in plane of triangle
		det = edge1.dotproduct(pvec);
		if (bCull){						// define TEST_CULL if culling is desired
			if (det < EPS)  return false;
			tvec.sub(C, p[0]);							// calculate distance from vert0 to ray origin
			u = tvec.dotproduct(pvec);			// calculate U parameter and test bounds
			if (u < 0.0f || u > det) return false;
			qvec.crossproduct(tvec, edge1);				// prepare to test V parameter
			v = D.dotproduct(qvec);			// calculate V parameter and test bounds
			if (v < 0.0f || u + v > det) return false;
			range = edge2.dotproduct(qvec);		// calculate t, scale parameters, ray intersects triangle
			inv_det = 1.0f / det;
			range *= inv_det;
			u *= inv_det;
			v *= inv_det;
		}else{											// the non-culling branch
			if (det > -EPS && det < EPS) return false;
			inv_det = 1.0f / det;
			tvec.sub(C, p[0]);							// calculate distance from vert0 to ray origin
			u = tvec.dotproduct(pvec)*inv_det;	// calculate U parameter and test bounds
			if (u < 0.0f || u > 1.0f)    return false;
			qvec.crossproduct(tvec, edge1);				// prepare to test V parameter
			v = D.dotproduct(qvec)*inv_det;	// calculate V parameter and test bounds
			if (v < 0.0f || u + v > 1.0f) return false;
			range = edge2.dotproduct(qvec)*inv_det;// calculate t, ray intersects triangle
		}
		return true;
	}

	//-- Ray-Triangle(always return range) : 1st level of indirection --------------------------------
	IC bool TestRayTri2(const Fvector& C, const Fvector& D, Fvector* p, float& range)
	{
		Fvector edge1, edge2, tvec, pvec, qvec;
		float det,inv_det,u,v;

		// find vectors for two edges sharing vert0
		edge1.sub(p[1], p[0]);
		edge2.sub(p[2], p[0]);
		// begin calculating determinant - also used to calculate U parameter
		pvec.crossproduct(D, edge2);
		// if determinant is near zero, ray lies in plane of triangle
		det = edge1.dotproduct(pvec);

		if (_abs(det) < EPS_S)		{ range=-1; return false; }
		inv_det = 1.0f / det;
		tvec.sub(C, p[0]);					// calculate distance from vert0 to ray origin
		u = tvec.dotproduct(pvec)*inv_det;	// calculate U parameter and test bounds
		qvec.crossproduct(tvec, edge1);		// prepare to test V parameter
		range = edge2.dotproduct(qvec)*inv_det;// calculate t, ray intersects plane
		if (u < 0.0f || u > 1.0f)		return false;
		v = D.dotproduct(qvec)*inv_det;		// calculate V parameter and test bounds
		if (v < 0.0f || u + v > 1.0f) return false;
		return true;
	}
	IC bool TestRayTri2(const Fvector& C, const Fvector& D, Fvector** p, float& range)
	{
		Fvector edge1, edge2, tvec, pvec, qvec;
		float det,inv_det,u,v;

		// find vectors for two edges sharing vert0
		edge1.sub(*p[1], *p[0]);
		edge2.sub(*p[2], *p[0]);
		// begin calculating determinant - also used to calculate U parameter
		pvec.crossproduct(D, edge2);
		// if determinant is near zero, ray lies in plane of triangle
		det = edge1.dotproduct(pvec);

		if (_abs(det) < EPS_S)		{ range=-1; return false; }
		inv_det = 1.0f / det;
		tvec.sub(C, *p[0]);					// calculate distance from vert0 to ray origin
		u = tvec.dotproduct(pvec)*inv_det;	// calculate U parameter and test bounds
		qvec.crossproduct(tvec, edge1);		// prepare to test V parameter
		range = edge2.dotproduct(qvec)*inv_det;// calculate t, ray intersects plane
		if (u < 0.0f || u > 1.0f)		return false;
		v = D.dotproduct(qvec)*inv_det;		// calculate V parameter and test bounds
		if (v < 0.0f || u + v > 1.0f) return false;
		return true;
	}
	//---------------------------------------------------------------------------
	// macros for fast arithmetic
	//---------------------------------------------------------------------------
	//---------------------------------------------------------------------------
	// compare [-r,r] to [NdD+dt*NdW]
#define TESTV0(NdD, R) \
    if		(NdD> R) return false;\
    else if	(NdD<-R) return false;
	//---------------------------------------------------------------------------
	// compare [-r,r] to [min{p,p+d0,p+d1},max{p,p+d0,p+d1}]
#define TESTV1(p,d0,d1,r){ \
    if ( (p) > (r) ){ \
	if ( (d0) >= 0.0f ){ \
	if ( (d1) >= 0.0f ){ \
				return false; \
	}else{ \
	if ( (p)+(d1) > (r) ) \
	return false; \
	} \
	}else if ( (d1) <= (d0) ){ \
	if ( (p)+(d1) > (r) ) \
	return false; \
	}else{ \
	if ( (p)+(d0) > (r) ) \
	return false; \
	} \
    }else if ( (p) < -(r) ){ \
	if ( (d0) <= 0.0f ){ \
	if ( (d1) <= 0.0f ){ \
				return false; \
	}else{ \
	if ( (p)+(d1) < -(r) ) \
	return false; \
	} \
	}else if ( (d1) >= (d0) ){ \
	if ( (p)+(d1) < -(r) ) \
	return false; \
	}else{ \
	if ( (p)+(d0) < -(r) ) \
	return false; \
	} \
    } \
	}
	//---------------------------------------------------------------------------
	// compare [-r,r] to [min{p,p+d},max{p,p+d}]
#define TESTV2(p,d,r){ \
    if ( (p) > (r) ){ \
	if ( (d) >= 0.0f ){ return false; \
	}else{ if ( (p)+(d) > (r) ) return false; } \
    }else if ( (p) < -(r) ){ \
	if ( (d) <= 0.0f ){ return false; \
	}else{ if ( (p)+(d) < -(r) ) return false; } \
    } \
	}
	//---------------------------------------------------------------------------

	IC bool TestBBoxTri(const Fmatrix33& A, const Fvector& T, const Fvector& extA, Fvector** p, BOOL bCulling){
		// construct triangle normal, difference of center and vertex (18 ops)
		Fvector D, E[2], N;
		E[0].sub(*p[1],*p[0]);
		E[1].sub(*p[2],*p[0]);
		N.crossproduct(E[0],E[1]);

		if (bCulling&&(A.k.dotproduct(N)>=0)) return false;

		D.sub(*p[0],T);

		// axis C+t*N
		float A0dN = A.i.dotproduct(N);
		float A1dN = A.j.dotproduct(N);
		float A2dN = A.k.dotproduct(N);
		float R = _abs(extA.x*A0dN)+_abs(extA.y*A1dN)+_abs(extA.z*A2dN);
		float NdD = N.dotproduct(D);
		TESTV0(NdD,R); //AXIS_N

		// axis C+t*A0
		float A0dD = A.i.dotproduct(D);
		float A0dE0 = A.i.dotproduct(E[0]);
		float A0dE1 = A.i.dotproduct(E[1]);
		TESTV1(A0dD,A0dE0,A0dE1,extA.x); //AXIS_A0

		// axis C+t*A1
		float A1dD	= A.j.dotproduct(D);
		float A1dE0 = A.j.dotproduct(E[0]);
		float A1dE1 = A.j.dotproduct(E[1]);
		TESTV1(A1dD,A1dE0,A1dE1,extA.y); //AXIS_A1

		// axis C+t*A2
		float A2dD	= A.k.dotproduct(D);
		float A2dE0 = A.k.dotproduct(E[0]);
		float A2dE1 = A.k.dotproduct(E[1]);
		TESTV1(A2dD,A2dE0,A2dE1,extA.z); //AXIS_A2

		// axis C+t*A0xE0
		Fvector A0xE0;
		A0xE0.crossproduct(A.i,E[0]);
		float A0xE0dD = A0xE0.dotproduct(D);
		R = _abs(extA.y*A2dE0)+_abs(extA.z*A1dE0);
		TESTV2(A0xE0dD,A0dN,R); //AXIS_A0xE0

		// axis C+t*A0xE1
		Fvector A0xE1;
		A0xE1.crossproduct(A.i,E[1]);
		float A0xE1dD = A0xE1.dotproduct(D);
		R = _abs(extA.y*A2dE1)+_abs(extA.z*A1dE1);
		TESTV2(A0xE1dD,-A0dN,R); //AXIS_A0xE1

		// axis C+t*A0xE2
		float A1dE2 = A1dE1-A1dE0;
		float A2dE2 = A2dE1-A2dE0;
		float A0xE2dD = A0xE1dD-A0xE0dD;
		R = _abs(extA.y*A2dE2)+_abs(extA.z*A1dE2);
		TESTV2(A0xE2dD,-A0dN,R); //AXIS_A0xE2

		// axis C+t*A1xE0
		Fvector A1xE0;
		A1xE0.crossproduct(A.j,E[0]);
		float A1xE0dD = A1xE0.dotproduct(D);
		R = _abs(extA.x*A2dE0)+_abs(extA.z*A0dE0);
		TESTV2(A1xE0dD,A1dN,R); //AXIS_A1xE0

		// axis C+t*A1xE1
		Fvector A1xE1;
		A1xE1.crossproduct(A.j,E[1]);
		float A1xE1dD = A1xE1.dotproduct(D);
		R = _abs(extA.x*A2dE1)+_abs(extA.z*A0dE1);
		TESTV2(A1xE1dD,-A1dN,R); //AXIS_A1xE1

		// axis C+t*A1xE2
		float A0dE2 = A0dE1-A0dE0;
		float A1xE2dD = A1xE1dD-A1xE0dD;
		R = _abs(extA.x*A2dE2)+_abs(extA.z*A0dE2);
		TESTV2(A1xE2dD,-A1dN,R); //AXIS_A1xE2

		// axis C+t*A2xE0
		Fvector A2xE0;
		A2xE0.crossproduct(A.k,E[0]);
		float A2xE0dD = A2xE0.dotproduct(D);
		R = _abs(extA.x*A1dE0)+_abs(extA.y*A0dE0);
		TESTV2(A2xE0dD,A2dN,R); //AXIS_A2xE0

		// axis C+t*A2xE1
		Fvector A2xE1;
		A2xE1.crossproduct(A.k,E[1]);
		float A2xE1dD = A2xE1.dotproduct(D);
		R = _abs(extA.x*A1dE1)+_abs(extA.y*A0dE1);
		TESTV2(A2xE1dD,-A2dN,R); //AXIS_A2xE1

		// axis C+t*A2xE2
		float A2xE2dD = A2xE1dD-A2xE0dD;
		R = _abs(extA.x*A1dE2)+_abs(extA.y*A0dE2);
		TESTV2(A2xE2dD,-A2dN,R); //AXIS_A2xE2

		// intersection occurs
		return true;
	}
	IC bool TestBBoxTri(const Fmatrix33& A, const Fvector& T, const Fvector& extA, Fvector* p, BOOL bCulling){
		// construct triangle normal, difference of center and vertex (18 ops)
		Fvector D, E[2], N;
		E[0].sub(p[1],p[0]);
		E[1].sub(p[2],p[0]);
		N.crossproduct(E[0],E[1]);

		if (bCulling&&(A.k.dotproduct(N)>=0)) return false;

		D.sub(p[0],T);

		// axis C+t*N
		float A0dN = A.i.dotproduct(N);
		float A1dN = A.j.dotproduct(N);
		float A2dN = A.k.dotproduct(N);
		float R = _abs(extA.x*A0dN)+_abs(extA.y*A1dN)+_abs(extA.z*A2dN);
		float NdD = N.dotproduct(D);
		TESTV0(NdD,R); //AXIS_N

		// axis C+t*A0
		float A0dD = A.i.dotproduct(D);
		float A0dE0 = A.i.dotproduct(E[0]);
		float A0dE1 = A.i.dotproduct(E[1]);
		TESTV1(A0dD,A0dE0,A0dE1,extA.x); //AXIS_A0

		// axis C+t*A1
		float A1dD	= A.j.dotproduct(D);
		float A1dE0 = A.j.dotproduct(E[0]);
		float A1dE1 = A.j.dotproduct(E[1]);
		TESTV1(A1dD,A1dE0,A1dE1,extA.y); //AXIS_A1

		// axis C+t*A2
		float A2dD	= A.k.dotproduct(D);
		float A2dE0 = A.k.dotproduct(E[0]);
		float A2dE1 = A.k.dotproduct(E[1]);
		TESTV1(A2dD,A2dE0,A2dE1,extA.z); //AXIS_A2

		// axis C+t*A0xE0
		Fvector A0xE0;
		A0xE0.crossproduct(A.i,E[0]);
		float A0xE0dD = A0xE0.dotproduct(D);
		R = _abs(extA.y*A2dE0)+_abs(extA.z*A1dE0);
		TESTV2(A0xE0dD,A0dN,R); //AXIS_A0xE0

		// axis C+t*A0xE1
		Fvector A0xE1;
		A0xE1.crossproduct(A.i,E[1]);
		float A0xE1dD = A0xE1.dotproduct(D);
		R = _abs(extA.y*A2dE1)+_abs(extA.z*A1dE1);
		TESTV2(A0xE1dD,-A0dN,R); //AXIS_A0xE1

		// axis C+t*A0xE2
		float A1dE2 = A1dE1-A1dE0;
		float A2dE2 = A2dE1-A2dE0;
		float A0xE2dD = A0xE1dD-A0xE0dD;
		R = _abs(extA.y*A2dE2)+_abs(extA.z*A1dE2);
		TESTV2(A0xE2dD,-A0dN,R); //AXIS_A0xE2

		// axis C+t*A1xE0
		Fvector A1xE0;
		A1xE0.crossproduct(A.j,E[0]);
		float A1xE0dD = A1xE0.dotproduct(D);
		R = _abs(extA.x*A2dE0)+_abs(extA.z*A0dE0);
		TESTV2(A1xE0dD,A1dN,R); //AXIS_A1xE0

		// axis C+t*A1xE1
		Fvector A1xE1;
		A1xE1.crossproduct(A.j,E[1]);
		float A1xE1dD = A1xE1.dotproduct(D);
		R = _abs(extA.x*A2dE1)+_abs(extA.z*A0dE1);
		TESTV2(A1xE1dD,-A1dN,R); //AXIS_A1xE1

		// axis C+t*A1xE2
		float A0dE2 = A0dE1-A0dE0;
		float A1xE2dD = A1xE1dD-A1xE0dD;
		R = _abs(extA.x*A2dE2)+_abs(extA.z*A0dE2);
		TESTV2(A1xE2dD,-A1dN,R); //AXIS_A1xE2

		// axis C+t*A2xE0
		Fvector A2xE0;
		A2xE0.crossproduct(A.k,E[0]);
		float A2xE0dD = A2xE0.dotproduct(D);
		R = _abs(extA.x*A1dE0)+_abs(extA.y*A0dE0);
		TESTV2(A2xE0dD,A2dN,R); //AXIS_A2xE0

		// axis C+t*A2xE1
		Fvector A2xE1;
		A2xE1.crossproduct(A.k,E[1]);
		float A2xE1dD = A2xE1.dotproduct(D);
		R = _abs(extA.x*A1dE1)+_abs(extA.y*A0dE1);
		TESTV2(A2xE1dD,-A2dN,R); //AXIS_A2xE1

		// axis C+t*A2xE2
		float A2xE2dD = A2xE1dD-A2xE0dD;
		R = _abs(extA.x*A1dE2)+_abs(extA.y*A0dE2);
		TESTV2(A2xE2dD,-A2dN,R); //AXIS_A2xE2

		// intersection occurs
		return true;
	}
	//---------------------------------------------------------------------------}

	//----------------------------------------------------------------------------
	IC float MgcSqrDistance (const Fvector& rkPoint, const Fvector& orig, const Fvector& e0,const Fvector& e1){

		Fvector kDiff;
		kDiff.sub(orig,rkPoint);

		float fA00 = e0.square_magnitude();
		float fA01 = e0.dotproduct(e1);
		float fA11 = e1.square_magnitude();
		float fB0 = kDiff.dotproduct(e0);
		float fB1 = kDiff.dotproduct(e1);
		float fC = kDiff.square_magnitude();
		float fDet = _abs(fA00*fA11-fA01*fA01);
		float fS = fA01*fB1-fA11*fB0;
		float fT = fA01*fB0-fA00*fB1;
		float fSqrDist;

		if ( fS + fT <= fDet ){
			if ( fS < 0.0f ){
				if ( fT < 0.0f ){  // region 4
					if ( fB0 < 0.0f ){
						fT = 0.0f;
						if ( -fB0 >= fA00 ){
							fS = 1.0f;
							fSqrDist = fA00+2.0f*fB0+fC;
						}else{
							fS = -fB0/fA00;
							fSqrDist = fB0*fS+fC;
						}
					}else{
						fS = 0.0f;
						if ( fB1 >= 0.0f ){
							fT = 0.0f;
							fSqrDist = fC;
						}else if ( -fB1 >= fA11 ){
							fT = 1.0f;
							fSqrDist = fA11+2.0f*fB1+fC;
						}else{
							fT = -fB1/fA11;
							fSqrDist = fB1*fT+fC;
						}
					}
				}else{  // region 3
					fS = 0.0f;
					if ( fB1 >= 0.0f ){
						fT = 0.0f;
						fSqrDist = fC;
					}else if ( -fB1 >= fA11 ){
						fT = 1;
						fSqrDist = fA11+2.0f*fB1+fC;
					}else{
						fT = -fB1/fA11;
						fSqrDist = fB1*fT+fC;
					}
				}
			}else if ( fT < 0.0f ){  // region 5
				fT = 0.0f;
				if ( fB0 >= 0.0f ){
					fS = 0.0f;
					fSqrDist = fC;
				}else if ( -fB0 >= fA00 ){
					fS = 1.0;
					fSqrDist = fA00+2.0f*fB0+fC;
				}else{
					fS = -fB0/fA00;
					fSqrDist = fB0*fS+fC;
				}
			}else{  // region 0
				// minimum at interior point
				float fInvDet = 1.0f/fDet;
				fS *= fInvDet;
				fT *= fInvDet;
				fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
					fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
			}
		}else{
			float fTmp0, fTmp1, fNumer, fDenom;

			if ( fS < 0.0f ){  // region 2
				fTmp0 = fA01 + fB0;
				fTmp1 = fA11 + fB1;
				if ( fTmp1 > fTmp0 ){
					fNumer = fTmp1 - fTmp0;
					fDenom = fA00-2.0f*fA01+fA11;
					if ( fNumer >= fDenom ){
						fS = 1.0f;
						fT = 0.0f;
						fSqrDist = fA00+2.0f*fB0+fC;
					}else{
						fS = fNumer/fDenom;
						fT = 1.0f - fS;
						fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
							fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
					}
				}else{
					fS = 0.0f;
					if ( fTmp1 <= 0.0f ){
						fT = 1.0f;
						fSqrDist = fA11+2.0f*fB1+fC;
					}else if ( fB1 >= 0.0f ){
						fT = 0.0f;
						fSqrDist = fC;
					}else{
						fT = -fB1/fA11;
						fSqrDist = fB1*fT+fC;
					}
				}
			}else if ( fT < 0.0 ){  // region 6
				fTmp0 = fA01 + fB1;
				fTmp1 = fA00 + fB0;
				if ( fTmp1 > fTmp0 ){
					fNumer = fTmp1 - fTmp0;
					fDenom = fA00-2.0f*fA01+fA11;
					if ( fNumer >= fDenom ){
						fT = 1.0f;
						fS = 0.0f;
						fSqrDist = fA11+2.0f*fB1+fC;
					}else{
						fT = fNumer/fDenom;
						fS = 1.0f - fT;
						fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
							fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
					}
				}else{
					fT = 0.0f;
					if ( fTmp1 <= 0.0f ){
						fS = 1.0f;
						fSqrDist = fA00+2.0f*fB0+fC;
					}else if ( fB0 >= 0.0f ){
						fS = 0.0f;
						fSqrDist = fC;
					}else{
						fS = -fB0/fA00;
						fSqrDist = fB0*fS+fC;
					}
				}
			}else{  // region 1
				fNumer = fA11 + fB1 - fA01 - fB0;
				if ( fNumer <= 0.0f ){
					fS = 0.0f;
					fT = 1.0f;
					fSqrDist = fA11+2.0f*fB1+fC;
				}else{
					fDenom = fA00-2.0f*fA01+fA11;
					if ( fNumer >= fDenom ){
						fS = 1.0f;
						fT = 0.0f;
						fSqrDist = fA00+2.0f*fB0+fC;
					}else{
						fS = fNumer/fDenom;
						fT = 1.0f - fS;
						fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
							fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
					}
				}
			}
		}

		return _abs(fSqrDist);
	}

	enum EST_Result{
		stNone		= 0,
		stIntersect	= 1,
		stInside	= 2,
	};

	IC EST_Result TestSphereTri(const Fvector& sphereOrigin, float sphereRadius,
						const Fvector& orig, const Fvector& e0,const Fvector& e1)
	{

		float fRSqr = sphereRadius*sphereRadius;
		Fvector kV0mC;
		kV0mC.sub(orig, sphereOrigin);

		// count the number of triangle vertices inside the sphere
		int iInside = 0;

		// test if v0 is inside the sphere
		if ( kV0mC.square_magnitude() <= fRSqr )
			iInside++;

		// test if v1 is inside the sphere
		Fvector kDiff;
		kDiff.add(kV0mC, e0);
		if ( kDiff.square_magnitude() <= fRSqr )
			iInside++;

		// test if v2 is inside the sphere
		kDiff.add(kV0mC, e1);
		if ( kDiff.square_magnitude() <= fRSqr )
			iInside++;

		// triangle does not traversely intersect sphere
		if ( iInside == 3 ) return stInside;

		// triangle transversely intersects sphere
		if ( iInside > 0 ) return stIntersect;

		// All vertices are outside the sphere, but the triangle might still
		// intersect the sphere.  This is the case when the distance from the
		// sphere center to the triangle is smaller than the radius.
		float fSqrDist = MgcSqrDistance(sphereOrigin,orig,e0,e1);
		return (fSqrDist < fRSqr)?stIntersect:stNone;
	}
	//---------------------------------------------------------------------------
	IC EST_Result TestSphereTri(const Fvector& sphereOrigin, float sphereRadius, Fvector* p)
	{
		Fvector e0, e1;
		// find vectors for two edges sharing vert0
		e0.sub(p[1], p[0]);
		e1.sub(p[2], p[0]);
		return TestSphereTri(sphereOrigin,sphereRadius,p[0],e0,e1);
	}
	IC EST_Result TestSphereTri(const Fvector& sphereOrigin, float sphereRadius, Fvector** p)
	{
		Fvector e0, e1;
		// find vectors for two edges sharing vert0
		e0.sub(*p[1], *p[0]);
		e1.sub(*p[2], *p[0]);
		return TestSphereTri(sphereOrigin,sphereRadius,*p[0],e0,e1);
	}
	IC bool TestSphereOBB(const Fsphere& rkSphere, const Fobb& rkBox)
	{
		// Test for intersection in the coordinate system of the box by
		// transforming the sphere into that coordinate system.
		Fvector3 kCDiff;
		kCDiff.sub(rkSphere.P,rkBox.m_translate);

		float fAx = _abs(kCDiff.dotproduct(rkBox.m_rotate.i));
		float fAy = _abs(kCDiff.dotproduct(rkBox.m_rotate.j));
		float fAz = _abs(kCDiff.dotproduct(rkBox.m_rotate.k));
		float fDx = fAx - rkBox.m_halfsize[0];
		float fDy = fAy - rkBox.m_halfsize[1];
		float fDz = fAz - rkBox.m_halfsize[2];

		if ( fAx <= rkBox.m_halfsize[0] ){
			if ( fAy <= rkBox.m_halfsize[1] ){
				if ( fAz <= rkBox.m_halfsize[2] ){
					// sphere center inside box
					return true;
				}else{
					// potential sphere-face intersection with face z
					return fDz <= rkSphere.R;
				}
			}else{
				if ( fAz <= rkBox.m_halfsize[2] ){
					// potential sphere-face intersection with face y
					return fDy <= rkSphere.R;
				}else{
					// potential sphere-edge intersection with edge formed
					// by faces y and z
					float fRSqr = rkSphere.R*rkSphere.R;
					return fDy*fDy + fDz*fDz <= fRSqr;
				}
			}
		}else{
			if ( fAy <= rkBox.m_halfsize[1] ){
				if ( fAz <= rkBox.m_halfsize[2] ){
					// potential sphere-face intersection with face x
					return fDx <= rkSphere.R;
				}else{
					// potential sphere-edge intersection with edge formed
					// by faces x and z
					float fRSqr = rkSphere.R*rkSphere.R;
					return fDx*fDx + fDz*fDz <= fRSqr;
				}
			}else{
				if ( fAz <= rkBox.m_halfsize[2] ){
					// potential sphere-edge intersection with edge formed
					// by faces x and y
					float fRSqr = rkSphere.R*rkSphere.R;
					return fDx*fDx + fDy*fDy <= fRSqr;
				}else{
					// potential sphere-vertex intersection at corner formed
					// by faces x,y,z
					float fRSqr = rkSphere.R*rkSphere.R;
					return fDx*fDx + fDy*fDy + fDz*fDz <= fRSqr;
				}
			}
		}
	}
	//----------------------------------------------------------------------------
	IC bool TestRayOBB (const Fvector3& origin, const Fvector3& direction, const Fobb& rkBox)
	{
		float			fWdU[3], fAWdU[3], fDdU[3], fADdU[3], fAWxDdU[3], fRhs;

		Fvector3		kDiff;
		kDiff.sub		(origin,rkBox.m_translate);

		fWdU[0]			= direction.dotproduct(rkBox.m_rotate.i);
		fAWdU[0]		= _abs(fWdU[0]);
		fDdU[0]			= kDiff.dotproduct(rkBox.m_rotate.i);
		fADdU[0]		= _abs(fDdU[0]);
		if ( fADdU[0] > rkBox.m_halfsize[0] && fDdU[0]*fWdU[0] >= (float)0.0 )
			return false;

		fWdU[1]			= direction.dotproduct(rkBox.m_rotate.j);
		fAWdU[1]		= _abs(fWdU[1]);
		fDdU[1]			= kDiff.dotproduct(rkBox.m_rotate.j);
		fADdU[1]		= _abs(fDdU[1]);
		if ( fADdU[1] > rkBox.m_halfsize[1] && fDdU[1]*fWdU[1] >= (float)0.0 )
			return false;

		fWdU[2]			= direction.dotproduct(rkBox.m_rotate.k);
		fAWdU[2]		= _abs(fWdU[2]);
		fDdU[2]			= kDiff.dotproduct(rkBox.m_rotate.k);
		fADdU[2]		= _abs(fDdU[2]);
		if ( fADdU[2] > rkBox.m_halfsize[2] && fDdU[2]*fWdU[2] >= (float)0.0 )
			return false;

		Fvector3		kWxD;
		kWxD.crossproduct(direction,kDiff);

		fAWxDdU[0]		= _abs(kWxD.dotproduct(rkBox.m_rotate.i));
		fRhs = rkBox.m_halfsize[1]*fAWdU[2] + rkBox.m_halfsize[2]*fAWdU[1];
		if ( fAWxDdU[0] > fRhs )
			return false;

		fAWxDdU[1]		= _abs(kWxD.dotproduct(rkBox.m_rotate.j));
		fRhs = rkBox.m_halfsize[0]*fAWdU[2] + rkBox.m_halfsize[2]*fAWdU[0];
		if ( fAWxDdU[1] > fRhs )
			return false;

		fAWxDdU[2]		= _abs(kWxD.dotproduct(rkBox.m_rotate.k));
		fRhs = rkBox.m_halfsize[0]*fAWdU[1] + rkBox.m_halfsize[1]*fAWdU[0];
		if ( fAWxDdU[2] > fRhs )
			return false;

		return true;
	}
//----------------------------------------------------------------------------
};


#endif
