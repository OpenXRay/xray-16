#include "stdafx.h"
#include "dCylinder.h"
// given a pointer `p' to a dContactGeom, return the dContactGeom at
// p + skip bytes.

#define M_SIN_PI_3		REAL(0.8660254037844386467637231707529362)
#define M_COS_PI_3		REAL(0.5000000000000000000000000000000000)

struct dxCylinder {	// cylinder
  dReal radius,lz;	// radius, length along y axis //
};

int dCylinderClassUser = -1;

#define NUMC_MASK (0xffff)

#define CONTACT(p,skip) ((dContactGeom*) (((char*)p) + (skip)))

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////circleIntersection//////////////////////////////////////////////////
//this does following:
//takes two circles as normals to planes n1,n2, center points cp1,cp2,and radiuses r1,r2
//finds line on which circles' planes intersect
//finds four points O1,O2 - intersection between the line and sphere with center cp1 radius r1
//					O3,O4 - intersection between the line and sphere with center cp2 radius r2
//returns false if there is no intersection
//computes distances O1-O3, O1-O4, O2-O3, O2-O4
//in "point" returns mean point between intersection points with smallest distance
/////////////////////////////////////////////////////////////////////////////////////////////////
inline bool circleIntersection(const dReal* n1,const dReal* cp1,dReal r1,const dReal* n2,const dReal* cp2,dReal r2,dVector3 point){
dReal c1=dDOT14(cp1,n1);
dReal c2=dDOT14(cp2,n2);
dReal _cos=dDOT44(n1,n2);
dReal cos_2=_cos*_cos;
dReal sin_2=1.f-cos_2;
dReal p1=(c1-c2*_cos)/sin_2;
dReal p2=(c2-c1*_cos)/sin_2;
dVector3 lp={p1*n1[0]+p2*n2[0],p1*n1[4]+p2*n2[4],p1*n1[8]+p2*n2[8]};
dVector3 n;
dCROSS144(n,=,n1,n2);
dVector3 LC1={lp[0]-cp1[0],lp[1]-cp1[1],lp[2]-cp1[2]};
dVector3 LC2={lp[0]-cp2[0],lp[1]-cp2[1],lp[2]-cp2[2]};
dReal A,B,C,B_A,B_A_2,D;
dReal t1,t2,t3,t4;
A=dDOT(n,n);
B=dDOT(LC1,n);
C=dDOT(LC1,LC1)-r1*r1;
B_A=B/A;
B_A_2=B_A*B_A;
D=B_A_2-C;
if(D<0.f){	//somewhat strange solution 
			//- it is needed to set some 
			//axis to sepparate cylinders
			//when their edges approach
	t1=-B_A+dSqrt(-D);
	t2=-B_A-dSqrt(-D);
//	return false;
	}
else{
t1=-B_A-dSqrt(D);
t2=-B_A+dSqrt(D);
}
B=dDOT(LC2,n);
C=dDOT(LC2,LC2)-r2*r2;
B_A=B/A;
B_A_2=B_A*B_A;
D=B_A_2-C;

if(D<0.f) {
	t3=-B_A+dSqrt(-D);
	t4=-B_A-dSqrt(-D);
//	return false;
	}
else{
t3=-B_A-dSqrt(D);
t4=-B_A+dSqrt(D);
}
dVector3 O1={lp[0]+n[0]*t1,lp[1]+n[1]*t1,lp[2]+n[2]*t1};
dVector3 O2={lp[0]+n[0]*t2,lp[1]+n[1]*t2,lp[2]+n[2]*t2};

dVector3 O3={lp[0]+n[0]*t3,lp[1]+n[1]*t3,lp[2]+n[2]*t3};
dVector3 O4={lp[0]+n[0]*t4,lp[1]+n[1]*t4,lp[2]+n[2]*t4};

dVector3 L1_3={O3[0]-O1[0],O3[1]-O1[1],O3[2]-O1[2]};
dVector3 L1_4={O4[0]-O1[0],O4[1]-O1[1],O4[2]-O1[2]};

dVector3 L2_3={O3[0]-O2[0],O3[1]-O2[1],O3[2]-O2[2]};
dVector3 L2_4={O4[0]-O2[0],O4[1]-O2[1],O4[2]-O2[2]};


dReal l1_3=dDOT(L1_3,L1_3);
dReal l1_4=dDOT(L1_4,L1_4);

dReal l2_3=dDOT(L2_3,L2_3);
dReal l2_4=dDOT(L2_4,L2_4);


if (l1_3<l1_4)
	if(l2_3<l2_4)
		if(l1_3<l2_3)
			{
			//l1_3;
			point[0]=0.5f*(O1[0]+O3[0]);
			point[1]=0.5f*(O1[1]+O3[1]);
			point[2]=0.5f*(O1[2]+O3[2]);
			}
		else{
			//l2_3;
			point[0]=0.5f*(O2[0]+O3[0]);
			point[1]=0.5f*(O2[1]+O3[1]);
			point[2]=0.5f*(O2[2]+O3[2]);
			}
	else
		if(l1_3<l2_4)
			{
			//l1_3;
			point[0]=0.5f*(O1[0]+O3[0]);
			point[1]=0.5f*(O1[1]+O3[1]);
			point[2]=0.5f*(O1[2]+O3[2]);
			}
		else{
			//l2_4;
			point[0]=0.5f*(O2[0]+O4[0]);
			point[1]=0.5f*(O2[1]+O4[1]);
			point[2]=0.5f*(O2[2]+O4[2]);
			}

else
	if(l2_3<l2_4)
		if(l1_4<l2_3)
			{
			//l1_4;
			point[0]=0.5f*(O1[0]+O4[0]);
			point[1]=0.5f*(O1[1]+O4[1]);
			point[2]=0.5f*(O1[2]+O4[2]);
			}
		else{
			//l2_3;
			point[0]=0.5f*(O2[0]+O3[0]);
			point[1]=0.5f*(O2[1]+O3[1]);
			point[2]=0.5f*(O2[2]+O3[2]);
			}
	else
		if(l1_4<l2_4)
			{
			//l1_4;
			point[0]=0.5f*(O1[0]+O4[0]);
			point[1]=0.5f*(O1[1]+O4[1]);
			point[2]=0.5f*(O1[2]+O4[2]);
			}
		else{
			//l2_4;
			point[0]=0.5f*(O2[0]+O4[0]);
			point[1]=0.5f*(O2[1]+O4[1]);
			point[2]=0.5f*(O2[2]+O4[2]);
			}

return true;
}




void lineClosestApproach (const dVector3 pa, const dVector3 ua,
				 const dVector3 pb, const dVector3 ub,
				 dReal *alpha, dReal *beta)
{
  dVector3 p;
  p[0] = pb[0] - pa[0];
  p[1] = pb[1] - pa[1];
  p[2] = pb[2] - pa[2];
  dReal uaub = dDOT(ua,ub);
  dReal q1 =  dDOT(ua,p);
  dReal q2 = -dDOT(ub,p);
  dReal d = 1-uaub*uaub;
  if (d <= 0) {
    // @@@ this needs to be made more robust
    *alpha = 0;
    *beta  = 0;
  }
  else {
    d = dRecip(d);
    *alpha = (q1 + uaub*q2)*d;
    *beta  = (uaub*q1 + q2)*d;
  }
}


// @@@ some stuff to optimize here, reuse code in contact point calculations.

extern "C" int dCylBox (const dVector3 p1, const dMatrix3 R1,
			const dReal radius,const dReal lz, const dVector3 p2,
			const dMatrix3 R2, const dVector3 side2,
			dVector3 normal, dReal *depth, int *code,
			int maxc, dContactGeom *contact, int skip)
{
  dVector3 p,pp,normalC;
  const dReal *normalR = 0;
  dReal B1,B2,B3,R11,R12,R13,R21,R22,R23,R31,R32,R33,
    Q11,Q12,Q13,Q21,Q22,Q23,Q31,Q32,Q33,s,s2,l,sQ21,sQ22,sQ23;
  int i,invert_normal;

  // get _vector_ from centers of box 1 to box 2, relative to box 1
  p[0] = p2[0] - p1[0];
  p[1] = p2[1] - p1[1];
  p[2] = p2[2] - p1[2];
  dMULTIPLY1_331 (pp,R1,p);		// get pp = p relative to body 1

  // get side lengths / 2
  //A1 =radius; A2 = lz*REAL(0.5); A3 = radius;
  dReal hlz=lz/2.f;
  B1 = side2[0]*REAL(0.5); B2 = side2[1]*REAL(0.5); B3 = side2[2]*REAL(0.5);

  // Rij is R1'*R2, i.e. the relative rotation between R1 and R2
  R11 = dDOT44(R1+0,R2+0); R12 = dDOT44(R1+0,R2+1); R13 = dDOT44(R1+0,R2+2);
  R21 = dDOT44(R1+1,R2+0); R22 = dDOT44(R1+1,R2+1); R23 = dDOT44(R1+1,R2+2);
  R31 = dDOT44(R1+2,R2+0); R32 = dDOT44(R1+2,R2+1); R33 = dDOT44(R1+2,R2+2);

  Q11 = dFabs(R11); Q12 = dFabs(R12); Q13 = dFabs(R13);
  Q21 = dFabs(R21); Q22 = dFabs(R22); Q23 = dFabs(R23);
  Q31 = dFabs(R31); Q32 = dFabs(R32); Q33 = dFabs(R33);

  
  //   * see if the axis separates the box with cylinder. if so, return 0.
  //   * find the depth of the penetration along the separating axis (s2)
  //   * if this is the largest depth so far, record it.
  // the normal vector3 will be set to the separating axis with the smallest
  // depth. note: normalR is set to point to a column of R1 or R2 if that is
  // the smallest depth normal so far. otherwise normalR is 0 and normalC is
  // set to a vector3 relative to body 1. invert_normal is 1 if the sign of
  // the normal should be flipped.

#define TEST(expr1,expr2,norm,cc) \
  s2 = dFabs(expr1) - (expr2); \
  if (s2 > 0) return 0; \
  if (s2 > s) { \
    s = s2; \
    normalR = norm; \
    invert_normal = ((expr1) < 0); \
    *code = (cc); \
  }

  s = -dInfinity;
  invert_normal = 0;
  *code = 0;

  // separating axis = cylinder ax u2
 //used when a box vertex touches a flat face of the cylinder
  TEST (pp[1],(hlz + B1*Q21 + B2*Q22 + B3*Q23),R1+1,0);


  // separating axis = box axis v1,v2,v3
  //used when cylinder edge touches box face
  //there is two ways to compute sQ: sQ21=dSqrt(1.f-Q21*Q21); or sQ21=dSqrt(Q23*Q23+Q22*Q22); 
  //if we did not need Q23 and Q22 the first way might be used to quiken the routine but then it need to 
  //check if Q21<=1.f, becouse it may slightly exeed 1.f.

 
  sQ21=dSqrt(Q23*Q23+Q22*Q22);
  TEST (dDOT41(R2+0,p),(radius*sQ21 + hlz*Q21 + B1),R2+0,1);

  sQ22=dSqrt(Q23*Q23+Q21*Q21);
  TEST (dDOT41(R2+1,p),(radius*sQ22 + hlz*Q22 + B2),R2+1,2);

  sQ23=dSqrt(Q22*Q22+Q21*Q21);
  TEST (dDOT41(R2+2,p),(radius*sQ23 + hlz*Q23 + B3),R2+2,3);

 
#undef TEST
#define TEST(expr1,expr2,n1,n2,n3,cc) \
  s2 = dFabs(expr1) - (expr2); \
  if (s2 > 0) return 0; \
  if (s2 > s) { \
      s = s2; \
	  normalR = 0; \
      normalC[0] = (n1); normalC[1] = (n2); normalC[2] = (n3); \
      invert_normal = ((expr1) < 0); \
      *code = (cc); \
    } 
 


// separating axis is a normal to the cylinder axis passing across the nearest box vertex
//used when a box vertex touches the lateral surface of the cylinder

dReal proj,boxProj,_cos,_sin,cos1,cos3;
dVector3 tAx,Ax,pb;
{
//making Ax which is perpendicular to cyl ax to box position//
proj=dDOT14(p2,R1+1)-dDOT14(p1,R1+1);

Ax[0]=p2[0]-p1[0]-R1[1]*proj;
Ax[1]=p2[1]-p1[1]-R1[5]*proj;
Ax[2]=p2[2]-p1[2]-R1[9]*proj;
dNormalize3(Ax);
//using Ax find box vertex which is nearest to the cylinder axis
	dReal sign;
    
    for (i=0; i<3; ++i) pb[i] = p2[i];
    sign = (dDOT14(Ax,R2+0) > 0) ? REAL(-1.0) : REAL(1.0);
    for (i=0; i<3; ++i) pb[i] += sign * B1 * R2[i*4];
    sign = (dDOT14(Ax,R2+1) > 0) ? REAL(-1.0) : REAL(1.0);
    for (i=0; i<3; ++i) pb[i] += sign * B2 * R2[i*4+1];
    sign = (dDOT14(Ax,R2+2) > 0) ? REAL(-1.0) : REAL(1.0);
    for (i=0; i<3; ++i) pb[i] += sign * B3 * R2[i*4+2];

//building axis which is normal to cylinder ax to the nearest box vertex
proj=dDOT14(pb,R1+1)-dDOT14(p1,R1+1);

Ax[0]=pb[0]-p1[0]-R1[1]*proj;
Ax[1]=pb[1]-p1[1]-R1[5]*proj;
Ax[2]=pb[2]-p1[2]-R1[9]*proj;
dNormalize3(Ax);
}

boxProj=dFabs(dDOT14(Ax,R2+0)*B1)+
		dFabs(dDOT14(Ax,R2+1)*B2)+
		dFabs(dDOT14(Ax,R2+2)*B3);

TEST(p[0]*Ax[0]+p[1]*Ax[1]+p[2]*Ax[2],(radius+boxProj),Ax[0],Ax[1],Ax[2],4);


//next three test used to handle collisions between cylinder circles and box ages
proj=dDOT14(p1,R2+0)-dDOT14(p2,R2+0);

tAx[0]=-p1[0]+p2[0]+R2[0]*proj;
tAx[1]=-p1[1]+p2[1]+R2[4]*proj;
tAx[2]=-p1[2]+p2[2]+R2[8]*proj;
dNormalize3(tAx);

//now tAx is normal to first ax of the box to cylinder center
//making perpendicular to tAx lying in the plane which is normal to the cylinder axis
//it is tangent in the point where projection of tAx on cylinder's ring intersect edge circle

_cos=dDOT14(tAx,R1+0);
_sin=dDOT14(tAx,R1+2);
tAx[0]=R1[2]*_cos-R1[0]*_sin;
tAx[1]=R1[6]*_cos-R1[4]*_sin;
tAx[2]=R1[10]*_cos-R1[8]*_sin;


//use cross between tAx and first ax of the box as separating axix 

dCROSS114(Ax,=,tAx,R2+0);
dNormalize3(Ax);

boxProj=dFabs(dDOT14(Ax,R2+1)*B2)+
		dFabs(dDOT14(Ax,R2+0)*B1)+
		dFabs(dDOT14(Ax,R2+2)*B3);

  _cos=dFabs(dDOT14(Ax,R1+1));
  cos1=dDOT14(Ax,R1+0);
  cos3=dDOT14(Ax,R1+2);
  _sin=dSqrt(cos1*cos1+cos3*cos3);

TEST(p[0]*Ax[0]+p[1]*Ax[1]+p[2]*Ax[2],(_sin*radius+_cos*hlz+boxProj),Ax[0],Ax[1],Ax[2],5);


//same thing with the second axis of the box
proj=dDOT14(p1,R2+1)-dDOT14(p2,R2+1);

tAx[0]=-p1[0]+p2[0]+R2[1]*proj;
tAx[1]=-p1[1]+p2[1]+R2[5]*proj;
tAx[2]=-p1[2]+p2[2]+R2[9]*proj;
dNormalize3(tAx);


_cos=dDOT14(tAx,R1+0);
_sin=dDOT14(tAx,R1+2);
tAx[0]=R1[2]*_cos-R1[0]*_sin;
tAx[1]=R1[6]*_cos-R1[4]*_sin;
tAx[2]=R1[10]*_cos-R1[8]*_sin;

dCROSS114(Ax,=,tAx,R2+1);
dNormalize3(Ax);

boxProj=dFabs(dDOT14(Ax,R2+0)*B1)+
		dFabs(dDOT14(Ax,R2+1)*B2)+
		dFabs(dDOT14(Ax,R2+2)*B3);

  _cos=dFabs(dDOT14(Ax,R1+1));
  cos1=dDOT14(Ax,R1+0);
  cos3=dDOT14(Ax,R1+2);
  _sin=dSqrt(cos1*cos1+cos3*cos3);
TEST(p[0]*Ax[0]+p[1]*Ax[1]+p[2]*Ax[2],(_sin*radius+_cos*hlz+boxProj),Ax[0],Ax[1],Ax[2],6);

//same thing with the third axis of the box
proj=dDOT14(p1,R2+2)-dDOT14(p2,R2+2);

Ax[0]=-p1[0]+p2[0]+R2[2]*proj;
Ax[1]=-p1[1]+p2[1]+R2[6]*proj;
Ax[2]=-p1[2]+p2[2]+R2[10]*proj;
dNormalize3(tAx);

_cos=dDOT14(tAx,R1+0);
_sin=dDOT14(tAx,R1+2);
tAx[0]=R1[2]*_cos-R1[0]*_sin;
tAx[1]=R1[6]*_cos-R1[4]*_sin;
tAx[2]=R1[10]*_cos-R1[8]*_sin;

dCROSS114(Ax,=,tAx,R2+2);
dNormalize3(Ax);
boxProj=dFabs(dDOT14(Ax,R2+1)*B2)+
		dFabs(dDOT14(Ax,R2+2)*B3)+
		dFabs(dDOT14(Ax,R2+0)*B1);

  _cos=dFabs(dDOT14(Ax,R1+1));
  cos1=dDOT14(Ax,R1+0);
  cos3=dDOT14(Ax,R1+2);
  _sin=dSqrt(cos1*cos1+cos3*cos3);
TEST(p[0]*Ax[0]+p[1]*Ax[1]+p[2]*Ax[2],(_sin*radius+_cos*hlz+boxProj),Ax[0],Ax[1],Ax[2],7);


#undef TEST

// note: cross product axes need to be scaled when s is computed.
// normal (n1,n2,n3) is relative to box 1.

#define TEST(expr1,expr2,n1,n2,n3,cc) \
  s2 = dFabs(expr1) - (expr2); \
  if (s2 > 0) return 0; \
  l = dSqrt ((n1)*(n1) + (n2)*(n2) + (n3)*(n3)); \
  if (l > 0) { \
    s2 /= l; \
    if (s2 > s) { \
      s = s2; \
      normalR = 0; \
      normalC[0] = (n1)/l; normalC[1] = (n2)/l; normalC[2] = (n3)/l; \
      invert_normal = ((expr1) < 0); \
      *code = (cc); \
    } \
  }

//crosses between cylinder axis and box axes
  // separating axis = u2 x (v1,v2,v3)
  TEST(pp[0]*R31-pp[2]*R11,(radius+B2*Q23+B3*Q22),R31,0,-R11,8);
  TEST(pp[0]*R32-pp[2]*R12,(radius+B1*Q23+B3*Q21),R32,0,-R12,9);
  TEST(pp[0]*R33-pp[2]*R13,(radius+B1*Q22+B2*Q21),R33,0,-R13,10);


#undef TEST

  // if we get to this point, the boxes interpenetrate. compute the normal
  // in global coordinates.
  if (normalR) {
    normal[0] = normalR[0];
    normal[1] = normalR[4];
    normal[2] = normalR[8];
  }
  else {
	  if(*code>7) dMULTIPLY0_331 (normal,R1,normalC);
	  else {normal[0] =normalC[0];normal[1] = normalC[1];normal[2] = normalC[2];}
  }

  if (invert_normal) {
    normal[0] = -normal[0];
    normal[1] = -normal[1];
    normal[2] = -normal[2];
  }
  *depth = -s;

  // compute contact point(s)

  if (*code > 7) {
 //find point on the cylinder pa deepest along normal
    dVector3 pa;
    dReal sign, cos1,cos3,factor;


    for (i=0; i<3; ++i) pa[i] = p1[i];

  	cos1 = dDOT14(normal,R1+0);
	cos3 = dDOT14(normal,R1+2) ;
	factor=dSqrt(cos1*cos1+cos3*cos3);

	cos1/=factor;
	cos3/=factor;
	
    for (i=0; i<3; ++i) pa[i] += cos1 * radius * R1[i*4];

    sign = (dDOT14(normal,R1+1) > 0) ? REAL(1.0) : REAL(-1.0);
    for (i=0; i<3; ++i) pa[i] += sign * hlz * R1[i*4+1];

  
    for (i=0; i<3; ++i) pa[i] += cos3 * radius * R1[i*4+2];

    // find vertex of the box  deepest along normal 
    dVector3 pb;
    for (i=0; i<3; ++i) pb[i] = p2[i];
    sign = (dDOT14(normal,R2+0) > 0) ? REAL(-1.0) : REAL(1.0);
    for (i=0; i<3; ++i) pb[i] += sign * B1 * R2[i*4];
    sign = (dDOT14(normal,R2+1) > 0) ? REAL(-1.0) : REAL(1.0);
    for (i=0; i<3; ++i) pb[i] += sign * B2 * R2[i*4+1];
    sign = (dDOT14(normal,R2+2) > 0) ? REAL(-1.0) : REAL(1.0);
    for (i=0; i<3; ++i) pb[i] += sign * B3 * R2[i*4+2];


    dReal alpha,beta;
    dVector3 ua,ub;
    for (i=0; i<3; ++i) ua[i] = R1[1 + i*4];
    for (i=0; i<3; ++i) ub[i] = R2[*code-8 + i*4];

    lineClosestApproach (pa,ua,pb,ub,&alpha,&beta);
    for (i=0; i<3; ++i) pa[i] += ua[i]*alpha;
    for (i=0; i<3; ++i) pb[i] += ub[i]*beta;

    for (i=0; i<3; ++i) contact[0].pos[i] = REAL(0.5)*(pa[i]+pb[i]);
    contact[0].depth = *depth;
    return 1;
  }


  	if(*code==4){
		for (i=0; i<3; ++i) contact[0].pos[i] = pb[i];
		contact[0].depth = *depth;
		return 1;
				}
  

  dVector3 vertex;
  if (*code == 0) {
   
    dReal sign;
    for (i=0; i<3; ++i) vertex[i] = p2[i];
    sign = (dDOT14(normal,R2+0) > 0) ? REAL(-1.0) : REAL(1.0);
    for (i=0; i<3; ++i) vertex[i] += sign * B1 * R2[i*4];
    sign = (dDOT14(normal,R2+1) > 0) ? REAL(-1.0) : REAL(1.0);
    for (i=0; i<3; ++i) vertex[i] += sign * B2 * R2[i*4+1];
    sign = (dDOT14(normal,R2+2) > 0) ? REAL(-1.0) : REAL(1.0);
    for (i=0; i<3; ++i) vertex[i] += sign * B3 * R2[i*4+2];
  }
  else {
   
    dReal sign,cos1,cos3,factor;
	dVector3 center;
     cos1 = dDOT14(normal,R1+0) ;
	cos3 = dDOT14(normal,R1+2);
	factor=dSqrt(cos1*cos1+cos3*cos3);
	factor= factor ? factor : 1.f;
	cos1/=factor;
	cos3/=factor;
	sign = (dDOT14(normal,R1+1) > 0) ? REAL(1.0) : REAL(-1.0);

	for (i=0; i<3; ++i) center[i] = p1[i]+sign * hlz * R1[i*4+1];
	for (i=0; i<3; ++i) vertex[i] = center[i]+cos1 * radius * R1[i*4];
	for (i=0; i<3; ++i) vertex[i] += cos3 * radius * R1[i*4+2];
	if(*code<4)
	{
			
			dReal A1,A3,centerDepth,Q1,Q3,sQ2;


			Q1=Q11;Q3=Q31;sQ2=sQ21;
			int ret=1;
			switch(*code) {
			//case 1:
			//	centerDepth=*depth-radius*sQ21;
			//	Q1=Q11;Q3=Q31;
			//	break;
			case 2:
				sQ2=sQ22;
				Q1=Q12;Q3=Q32;
				break;
			case 3:
				sQ2=sQ23;
				Q1=Q13;Q3=Q33;
				break;
			
			}
			
			if(sQ2<M_SQRT1_2)
			{
			
			centerDepth=*depth-radius*sQ2;
			A1=(-cos1*M_COS_PI_3-cos3*M_SIN_PI_3)*radius;
			A3=(-cos3*M_COS_PI_3+cos1*M_SIN_PI_3)*radius;
			CONTACT(contact,ret*skip)->pos[0]=center[0]+A1*R1[0]+A3*R1[2];
			CONTACT(contact,ret*skip)->pos[1]=center[1]+A1*R1[4]+A3*R1[6];
			CONTACT(contact,ret*skip)->pos[2]=center[2]+A1*R1[8]+A3*R1[10];
			CONTACT(contact,ret*skip)->depth=centerDepth+Q1*A1+Q3*A3;

			if(CONTACT(contact,ret*skip)->depth>0.f)++ret;

			A1=(-cos1*M_COS_PI_3+cos3*M_SIN_PI_3)*radius;
			A3=(-cos3*M_COS_PI_3-cos1*M_SIN_PI_3)*radius;
			CONTACT(contact,ret*skip)->pos[0]=center[0]+A1*R1[0]+A3*R1[2];
			CONTACT(contact,ret*skip)->pos[1]=center[1]+A1*R1[4]+A3*R1[6];
			CONTACT(contact,ret*skip)->pos[2]=center[2]+A1*R1[8]+A3*R1[10];
			CONTACT(contact,ret*skip)->depth=centerDepth+Q1*A1+Q3*A3;

			if(CONTACT(contact,ret*skip)->depth>0.f)++ret;
			}

			for (i=0; i<3; ++i) contact[0].pos[i] = vertex[i];
			contact[0].depth = *depth;
		return ret;
	}

  }
  for (i=0; i<3; ++i) contact[0].pos[i] = vertex[i];
  contact[0].depth = *depth;
  return 1;
}

//****************************************************************************

extern "C" int dCylCyl (const dVector3 p1, const dMatrix3 R1,
			const dReal radius1,const dReal lz1, const dVector3 p2,
			const dMatrix3 R2, const dReal radius2,const dReal lz2,
			dVector3 normal, dReal *depth, int *code,
			int maxc, dContactGeom *contact, int skip)
{
  dVector3 p,pp1,pp2,normalC;
  const dReal *normalR = 0;
  dReal hlz1,hlz2,s,s2;
  int i,invert_normal;

  // get vector3 from centers of box 1 to box 2, relative to box 1
  p[0] = p2[0] - p1[0];
  p[1] = p2[1] - p1[1];
  p[2] = p2[2] - p1[2];
  dMULTIPLY1_331 (pp1,R1,p);		// get pp1 = p relative to body 1
  dMULTIPLY1_331 (pp2,R2,p);
  // get side lengths / 2
  hlz1 = lz1*REAL(0.5);
  hlz2 = lz2*REAL(0.5); 

 dReal proj,cos1,cos3;



#define TEST(expr1,expr2,norm,cc) \
  s2 = dFabs(expr1) - (expr2); \
  if (s2 > 0) return 0; \
  if (s2 > s) { \
    s = s2; \
    normalR = norm; \
    invert_normal = ((expr1) < 0); \
    *code = (cc); \
  }

  s = -dInfinity;
  invert_normal = 0;
  *code = 0;

  dReal c_cos=dFabs(dDOT44(R1+1,R2+1));
  dReal c_sin=dSqrt(1.f-(c_cos>1.f ? 1.f : c_cos));

  TEST (pp1[1],(hlz1 + radius2*c_sin + hlz2*c_cos ),R1+1,0);//pp

 /// TEST (pp2[1],(radius1*c_sin + hlz1*c_cos + hlz2),R2+1,1);



  // note: cross product axes need to be scaled when s is computed.
 
#undef TEST
#define TEST(expr1,expr2,n1,n2,n3,cc) \
  s2 = dFabs(expr1) - (expr2); \
  if (s2 > 0) return 0; \
  if (s2 > s) { \
      s = s2; \
	  normalR = 0; \
      normalC[0] = (n1); normalC[1] = (n2); normalC[2] = (n3); \
      invert_normal = ((expr1) < 0); \
      *code = (cc); \
    } 
 

dVector3 tAx,Ax,pa,pb;

//cross between cylinders' axes
dCROSS144(Ax,=,R1+1,R2+1);
dNormalize3(Ax);
TEST(p[0]*Ax[0]+p[1]*Ax[1]+p[2]*Ax[2],radius1+radius2,Ax[0],Ax[1],Ax[2],6);


{
 
    dReal sign, factor;

	//making ax which is perpendicular to cyl1 ax passing across cyl2 position//
		//(project p on cyl1 flat surface )
    for (i=0; i<3; ++i) pb[i] = p2[i];
 	//cos1 = dDOT14(p,R1+0);
	//cos3 = dDOT14(p,R1+2) ;
	tAx[0]=pp1[0]*R1[0]+pp1[2]*R1[2];
	tAx[1]=pp1[0]*R1[4]+pp1[2]*R1[6];
	tAx[2]=pp1[0]*R1[8]+pp1[2]*R1[10];
	dNormalize3(tAx);

//find deepest point pb of cyl2 on opposite direction of tAx
 	cos1 = dDOT14(tAx,R2+0);
	cos3 = dDOT14(tAx,R2+2) ;
	factor=dSqrt(cos1*cos1+cos3*cos3);
	cos1/=factor;
	cos3/=factor;
    for (i=0; i<3; ++i) pb[i] -= cos1 * radius2 * R2[i*4];

    sign = (dDOT14(tAx,R2+1) > 0) ? REAL(1.0) : REAL(-1.0);
    for (i=0; i<3; ++i) pb[i] -= sign * hlz2 * R2[i*4+1];

    for (i=0; i<3; ++i) pb[i] -= cos3 * radius2 * R2[i*4+2];

//making perpendicular to cyl1 ax passing across pb
	proj=dDOT14(pb,R1+1)-dDOT14(p1,R1+1);

	Ax[0]=pb[0]-p1[0]-R1[1]*proj;
	Ax[1]=pb[1]-p1[1]-R1[5]*proj;
	Ax[2]=pb[2]-p1[2]-R1[9]*proj;

}

dNormalize3(Ax);


  dReal _cos=dFabs(dDOT14(Ax,R2+1));
  cos1=dDOT14(Ax,R2+0);
  cos3=dDOT14(Ax,R2+2);
  dReal _sin=dSqrt(cos1*cos1+cos3*cos3);

TEST(p[0]*Ax[0]+p[1]*Ax[1]+p[2]*Ax[2],radius1+_cos*hlz2+_sin*radius2,Ax[0],Ax[1],Ax[2],3);



{
   
   dReal sign, factor;
   	
    for (i=0; i<3; ++i) pa[i] = p1[i];

 	//making ax which is perpendicular to cyl2 ax passing across cyl1 position//
	//(project p on cyl2 flat surface )
 	//cos1 = dDOT14(p,R2+0);
	//cos3 = dDOT14(p,R2+2) ;
	tAx[0]=pp2[0]*R2[0]+pp2[2]*R2[2];
	tAx[1]=pp2[0]*R2[4]+pp2[2]*R2[6];
	tAx[2]=pp2[0]*R2[8]+pp2[2]*R2[10];
	dNormalize3(tAx);

 	cos1 = dDOT14(tAx,R1+0);
	cos3 = dDOT14(tAx,R1+2) ;
	factor=dSqrt(cos1*cos1+cos3*cos3);
	cos1/=factor;
	cos3/=factor;

//find deepest point pa of cyl2 on direction of tAx
    for (i=0; i<3; ++i) pa[i] += cos1 * radius1 * R1[i*4];

    sign = (dDOT14(tAx,R1+1) > 0) ? REAL(1.0) : REAL(-1.0);
    for (i=0; i<3; ++i) pa[i] += sign * hlz1 * R1[i*4+1];

  
    for (i=0; i<3; ++i) pa[i] += cos3 * radius1 * R1[i*4+2];

	proj=dDOT14(pa,R2+1)-dDOT14(p2,R2+1);

	Ax[0]=pa[0]-p2[0]-R2[1]*proj;
	Ax[1]=pa[1]-p2[1]-R2[5]*proj;
	Ax[2]=pa[2]-p2[2]-R2[9]*proj;

}
dNormalize3(Ax);



  _cos=dFabs(dDOT14(Ax,R1+1));
  cos1=dDOT14(Ax,R1+0);
  cos3=dDOT14(Ax,R1+2);
  _sin=dSqrt(cos1*cos1+cos3*cos3);

TEST(p[0]*Ax[0]+p[1]*Ax[1]+p[2]*Ax[2],radius2+_cos*hlz1+_sin*radius1,Ax[0],Ax[1],Ax[2],4);


////test circl

//@ this needed to set right normal when cylinders edges intersect
//@ the most precise axis for this test may be found as a line between nearest points of two
//@ circles. But it needs comparatively a lot of computation.
//@ I use a trick which lets not to solve quadric equation. 
//@ In the case when cylinder eidges touches the test below rather accurate.
//@ I still not sure about problems with sepparation but they have not been revealed during testing.
dVector3 point;
{
 dVector3 ca,cb; 
 dReal sign;
 for (i=0; i<3; ++i) ca[i] = p1[i];
 for (i=0; i<3; ++i) cb[i] = p2[i];
//find two nearest flat rings
 sign = (pp1[1] > 0) ? REAL(1.0) : REAL(-1.0);
 for (i=0; i<3; ++i) ca[i] += sign * hlz1 * R1[i*4+1];

 sign = (pp2[1] > 0) ? REAL(1.0) : REAL(-1.0);
 for (i=0; i<3; ++i) cb[i] -= sign * hlz2 * R2[i*4+1];

 dVector3 tAx,tAx1;
	circleIntersection(R1+1,ca,radius1,R2+1,cb,radius2,point);

	Ax[0]=point[0]-ca[0];
	Ax[1]=point[1]-ca[1];
	Ax[2]=point[2]-ca[2];

  	cos1 = dDOT14(Ax,R1+0);
	cos3 = dDOT14(Ax,R1+2) ;

	tAx[0]=cos3*R1[0]-cos1*R1[2];
	tAx[1]=cos3*R1[4]-cos1*R1[6];
	tAx[2]=cos3*R1[8]-cos1*R1[10];

	Ax[0]=point[0]-cb[0];
	Ax[1]=point[1]-cb[1];
	Ax[2]=point[2]-cb[2];


 	cos1 = dDOT14(Ax,R2+0);
	cos3 = dDOT14(Ax,R2+2) ;

	tAx1[0]=cos3*R2[0]-cos1*R2[2];
	tAx1[1]=cos3*R2[4]-cos1*R2[6];
	tAx1[2]=cos3*R2[8]-cos1*R2[10];
	dCROSS(Ax,=,tAx,tAx1);
	

 

dNormalize3(Ax);
dReal cyl1Pr,cyl2Pr;

 _cos=dFabs(dDOT14(Ax,R1+1));
 cos1=dDOT14(Ax,R1+0);
 cos3=dDOT14(Ax,R1+2);
 _sin=dSqrt(cos1*cos1+cos3*cos3);
 cyl1Pr=_cos*hlz1+_sin*radius1;

 _cos=dFabs(dDOT14(Ax,R2+1));
 cos1=dDOT14(Ax,R2+0);
 cos3=dDOT14(Ax,R2+2);
 _sin=dSqrt(cos1*cos1+cos3*cos3);
 cyl2Pr=_cos*hlz2+_sin*radius2;
TEST(p[0]*Ax[0]+p[1]*Ax[1]+p[2]*Ax[2],cyl1Pr+cyl2Pr,Ax[0],Ax[1],Ax[2],5);


}


#undef TEST



  // if we get to this point, the cylinders interpenetrate. compute the normal
  // in global coordinates.
  if (normalR) {
    normal[0] = normalR[0];
    normal[1] = normalR[4];
    normal[2] = normalR[8];
  }
  else {
		normal[0] =normalC[0];normal[1] = normalC[1];normal[2] = normalC[2];
		}
  if (invert_normal) {
    normal[0] = -normal[0];
    normal[1] = -normal[1];
    normal[2] = -normal[2];
  }

  *depth = -s;

  // compute contact point(s)

	if(*code==3){
		for (i=0; i<3; ++i) contact[0].pos[i] = pb[i];
		contact[0].depth = *depth;
		return 1;
				}

	if(*code==4){
		for (i=0; i<3; ++i) contact[0].pos[i] = pa[i];
		contact[0].depth = *depth;
		return 1;
				}

	if(*code==5){
		for (i=0; i<3; ++i) contact[0].pos[i] = point[i];
		contact[0].depth = *depth;
		return 1;
				}

if (*code == 6) {
	    dVector3 pa;
    dReal sign, cos1,cos3,factor;


    for (i=0; i<3; ++i) pa[i] = p1[i];

  	cos1 = dDOT14(normal,R1+0);
	cos3 = dDOT14(normal,R1+2) ;
	factor=dSqrt(cos1*cos1+cos3*cos3);
	if(factor>0.f)
	{
		cos1/=factor;
		cos3/=factor;
	}
    for (i=0; i<3; ++i) pa[i] += cos1 * radius1 * R1[i*4];

    sign = (dDOT14(normal,R1+1) > 0) ? REAL(1.0) : REAL(-1.0);
    for (i=0; i<3; ++i) pa[i] += sign * hlz1 * R1[i*4+1];

  
    for (i=0; i<3; ++i) pa[i] += cos3 * radius1 * R1[i*4+2];

    // find a point pb on the intersecting edge of cylinder 2
    dVector3 pb;
    for (i=0; i<3; ++i) pb[i] = p2[i];
 	cos1 = dDOT14(normal,R2+0);
	cos3 = dDOT14(normal,R2+2) ;
	factor=dSqrt(cos1*cos1+cos3*cos3);
	if(factor>0.f)
	{
		cos1/=factor;
		cos3/=factor;
	}
    for (i=0; i<3; ++i) pb[i] -= cos1 * radius2 * R2[i*4];

    sign = (dDOT14(normal,R2+1) > 0) ? REAL(1.0) : REAL(-1.0);
    for (i=0; i<3; ++i) pb[i] -= sign * hlz2 * R2[i*4+1];

  
    for (i=0; i<3; ++i) pb[i] -= cos3 * radius2 * R2[i*4+2];

	
	dReal alpha,beta;
	dVector3 ua,ub;
	for (i=0; i<3; ++i) ua[i] = R1[1 + i*4];
	for (i=0; i<3; ++i) ub[i] = R2[1 + i*4];
	lineClosestApproach (pa,ua,pb,ub,&alpha,&beta);
	for (i=0; i<3; ++i) pa[i] += ua[i]*alpha;
	for (i=0; i<3; ++i) pb[i] += ub[i]*beta;

    for (i=0; i<3; ++i) contact[0].pos[i] = REAL(0.5)*(pa[i]+pb[i]);
    contact[0].depth = *depth;
    return 1;
  }

  // okay, we have a face-something intersection (because the separating
  // axis is perpendicular to a face).

  // @@@ temporary: make deepest point on the "other" cylinder the contact point.
  // @@@ this kind of works, but we need multiple contact points for stability,
  // @@@ especially for face-face contact.
  
  dVector3 vertex;
  int ret=1;
  if (*code == 0) {

    // flat face from cylinder 1 touches a edge/face from cylinder 2.
    dReal sign,cos1,cos3,factor;
   // for (i=0; i<3; ++i) vertex[i] = p2[i];
    cos1 = dDOT14(normal,R2+0) ;
	cos3 = dDOT14(normal,R2+2);
	factor=dSqrt(cos1*cos1+cos3*cos3);
	if(factor>0.f)
	{
		cos1/=factor;
		cos3/=factor;
	}
	dVector3 center;

	sign = (dDOT14(normal,R2+1) > 0) ? REAL(1.0) : REAL(-1.0);
	for (i=0; i<3; ++i) center[i] =p2[i]- sign * hlz2 * R2[i*4+1];

    for (i=0; i<3; ++i) vertex[i] =center[i]- cos1 * radius2 * R2[i*4];

    for (i=0; i<3; ++i) vertex[i] -=cos3 * radius2 * R2[i*4+2];
	

	dReal A1,A3,centerDepth,Q1,Q3;
	centerDepth=*depth-radius2*(factor);
	Q1=-(dDOT14(normal,R2+0));Q3=-(dDOT14(normal,R2+2));

	A1=-(-cos1*M_COS_PI_3-cos3*M_SIN_PI_3)*radius2;
	A3=-(-cos3*M_COS_PI_3+cos1*M_SIN_PI_3)*radius2;
	CONTACT(contact,ret*skip)->pos[0]=center[0]+A1*R2[0]+A3*R2[2];
	CONTACT(contact,ret*skip)->pos[1]=center[1]+A1*R2[4]+A3*R2[6];
	CONTACT(contact,ret*skip)->pos[2]=center[2]+A1*R2[8]+A3*R2[10];
	CONTACT(contact,ret*skip)->depth=centerDepth+(Q1*A1)+(Q3*A3);

	if(CONTACT(contact,ret*skip)->depth>0.f)++ret;

	A1=-(-cos1*M_COS_PI_3+cos3*M_SIN_PI_3)*radius2;
	A3=-(-cos3*M_COS_PI_3-cos1*M_SIN_PI_3)*radius2;
	CONTACT(contact,ret*skip)->pos[0]=center[0]+A1*R2[0]+A3*R2[2];
	CONTACT(contact,ret*skip)->pos[1]=center[1]+A1*R2[4]+A3*R2[6];
	CONTACT(contact,ret*skip)->pos[2]=center[2]+A1*R2[8]+A3*R2[10];
	CONTACT(contact,ret*skip)->depth=centerDepth+(Q1*A1)+(Q3*A3);

	if(CONTACT(contact,ret*skip)->depth>0.f)++ret;

  }
  else {
     // flat face from cylinder 2 touches a edge/face from cylinder 1.
    dReal sign,cos1,cos3,factor;
   // for (i=0; i<3; ++i) vertex[i] = p1[i];
    cos1 = dDOT14(normal,R1+0) ;
	cos3 = dDOT14(normal,R1+2);
	factor=dSqrt(cos1*cos1+cos3*cos3);
	if(factor>0.f)
	{
		cos1/=factor;
		cos3/=factor;
	}

	dVector3 center;

	sign = (dDOT14(normal,R1+1) > 0) ? REAL(1.0) : REAL(-1.0);
	for (i=0; i<3; ++i) center[i] =p1[i]+sign * hlz1 * R1[i*4+1];


    for (i=0; i<3; ++i) vertex[i] =center[i]+cos1 * radius1 * R1[i*4];
    for (i=0; i<3; ++i) vertex[i] += cos3 * radius1 * R1[i*4+2];


	dReal A1,A3,centerDepth,Q1,Q3;
	centerDepth=*depth-radius1*(factor);
	Q1=(dDOT(R2+1,R1+0));Q3=(dDOT(R2+1,R1+2));


	A1=(-cos1*M_COS_PI_3-cos3*M_SIN_PI_3)*radius1;
	A3=(-cos3*M_COS_PI_3+cos1*M_SIN_PI_3)*radius1;
	CONTACT(contact,ret*skip)->pos[0]=center[0]+A1*R1[0]+A3*R1[2];
	CONTACT(contact,ret*skip)->pos[1]=center[1]+A1*R1[4]+A3*R1[6];
	CONTACT(contact,ret*skip)->pos[2]=center[2]+A1*R1[8]+A3*R1[10];
	CONTACT(contact,ret*skip)->depth=centerDepth+dFabs(Q1*A1)+dFabs(Q3*A3);

	if(CONTACT(contact,ret*skip)->depth>0.f)++ret;

	A1=(-cos1*M_COS_PI_3+cos3*M_SIN_PI_3)*radius1;
	A3=(-cos3*M_COS_PI_3-cos1*M_SIN_PI_3)*radius1;
	CONTACT(contact,ret*skip)->pos[0]=center[0]+A1*R1[0]+A3*R1[2];
	CONTACT(contact,ret*skip)->pos[1]=center[1]+A1*R1[4]+A3*R1[6];
	CONTACT(contact,ret*skip)->pos[2]=center[2]+A1*R1[8]+A3*R1[10];
	CONTACT(contact,ret*skip)->depth=centerDepth+dFabs(Q1*A1)+dFabs(Q3*A3);

	if(CONTACT(contact,ret*skip)->depth>0.f)++ret;


  }
  for (i=0; i<3; ++i) contact[0].pos[i] = vertex[i];
  contact[0].depth = *depth;
  return ret;
}

#pragma todo(optimize factor==0.f)
//****************************************************************************


int dCollideCylS (dxGeom *o1, dxGeom *o2, int flags,
		dContactGeom *contact, int skip)
{
 

  VERIFY (skip >= (int)sizeof(dContactGeom));
  VERIFY (dGeomGetClass(o2) == dSphereClass);
  VERIFY (dGeomGetClass(o1) == dCylinderClassUser);
  const dReal* p1=dGeomGetPosition(o1);
  const dReal* p2=dGeomGetPosition(o2);
  const dReal* R=dGeomGetRotation(o1);
  dVector3 p,normalC,normal;
  const dReal *normalR = 0;
  dReal cylRadius;
  dReal hl;
  dGeomCylinderGetParams(o1,&cylRadius,&hl);
  hl/=2.f;
  dReal sphereRadius;
  sphereRadius=dGeomSphereGetRadius(o2);
  
  int i,invert_normal;

  // get vector3 from centers of cyl to shere
  p[0] = p2[0] - p1[0];
  p[1] = p2[1] - p1[1];
  p[2] = p2[2] - p1[2];
 
dReal s,s2;
unsigned char code;
#define TEST(expr1,expr2,norm,cc) \
  s2 = dFabs(expr1) - (expr2); \
  if (s2 > 0) return 0; \
  if (s2 > s) { \
    s = s2; \
    normalR = norm; \
    invert_normal = ((expr1) < 0); \
    code = (cc); \
  }

  s = -dInfinity;
  invert_normal = 0;
  code = 0;

  // separating axis cyl ax 

  TEST (dDOT14(p,R+1),sphereRadius+hl,R+1,2);
  // note: cross product axes need to be scaled when s is computed.
  // normal (n1,n2,n3) is relative to 
#undef TEST
#define TEST(expr1,expr2,n1,n2,n3,cc) \
  s2 = dFabs(expr1) - (expr2); \
  if (s2 > 0) return 0; \
  if (s2 > s) { \
      s = s2; \
	  normalR = 0; \
      normalC[0] = (n1); normalC[1] = (n2); normalC[2] = (n3); \
      invert_normal = ((expr1) < 0); \
      code = (cc); \
    } 
 
//making ax which is perpendicular to cyl1 ax to sphere center//
 
dReal proj,_cos,_sin,cos1,cos3;
dVector3 Ax;
	proj=dDOT14(p2,R+1)-dDOT14(p1,R+1);

	Ax[0]=p2[0]-p1[0]-R[1]*proj;
	Ax[1]=p2[1]-p1[1]-R[5]*proj;
	Ax[2]=p2[2]-p1[2]-R[9]*proj;
dNormalize3(Ax);
TEST(dDOT(p,Ax),sphereRadius+cylRadius,Ax[0],Ax[1],Ax[2],9);


Ax[0]=p[0];
Ax[1]=p[1];
Ax[2]=p[2];
dNormalize3(Ax);

	dVector3 pa;
    dReal sign, factor;
    for (i=0; i<3; ++i) pa[i] = p1[i];

  	cos1 = dDOT14(Ax,R+0);
	cos3 = dDOT14(Ax,R+2) ;
	factor=dSqrt(cos1*cos1+cos3*cos3);
	cos1/=factor;
	cos3/=factor;
    for (i=0; i<3; ++i) pa[i] += cos1 * cylRadius * R[i*4];
    sign = (dDOT14(Ax,R+1) > 0) ? REAL(1.0) : REAL(-1.0);
    for (i=0; i<3; ++i) pa[i] += sign * hl * R[i*4+1];
    for (i=0; i<3; ++i) pa[i] += cos3 * cylRadius  * R[i*4+2];

Ax[0]=p2[0]-pa[0];
Ax[1]=p2[1]-pa[1];
Ax[2]=p2[2]-pa[2];
dNormalize3(Ax);

 _cos=dFabs(dDOT14(Ax,R+1));
 cos1=dDOT14(Ax,R+0);
 cos3=dDOT14(Ax,R+2);
 _sin=dSqrt(cos1*cos1+cos3*cos3);
TEST(dDOT(p,Ax),sphereRadius+cylRadius*_sin+hl*_cos,Ax[0],Ax[1],Ax[2],14);


#undef TEST

  if (normalR) {
    normal[0] = normalR[0];
    normal[1] = normalR[4];
    normal[2] = normalR[8];
  }
  else {

	normal[0] = normalC[0];
	normal[1] = normalC[1];
	normal[2] = normalC[2];
		}
  if (invert_normal) {
    normal[0] = -normal[0];
    normal[1] = -normal[1];
    normal[2] = -normal[2];
  }
   // compute contact point(s)
contact->depth=-s;
contact->normal[0]=-normal[0];
contact->normal[1]=-normal[1];
contact->normal[2]=-normal[2];
contact->g1=const_cast<dxGeom*> (o1);
contact->g2=const_cast<dxGeom*> (o2);
contact->pos[0]=p2[0]-normal[0]*sphereRadius;
contact->pos[1]=p2[1]-normal[1]*sphereRadius;
contact->pos[2]=p2[2]-normal[2]*sphereRadius;
return 1;
}



int dCollideCylB (dxGeom *o1, dxGeom *o2, int flags,
		dContactGeom *contact, int skip)
{
  dVector3 normal;
  dReal depth;
  int code;
  dReal cylRadius,cylLength;
  dVector3 boxSides;
  dGeomCylinderGetParams(o1,&cylRadius,&cylLength);
  dGeomBoxGetLengths(o2,boxSides);
  int num = dCylBox(dGeomGetPosition(o1),dGeomGetRotation(o1),cylRadius,cylLength, 
					dGeomGetPosition(o2),dGeomGetRotation(o2),boxSides,
					normal,&depth,&code,flags & NUMC_MASK,contact,skip);
  for (int i=0; i<num; ++i) {
    CONTACT(contact,i*skip)->normal[0] = -normal[0];
    CONTACT(contact,i*skip)->normal[1] = -normal[1];
    CONTACT(contact,i*skip)->normal[2] = -normal[2];
    CONTACT(contact,i*skip)->g1 = const_cast<dxGeom*> (o1);
    CONTACT(contact,i*skip)->g2 = const_cast<dxGeom*> (o2);
  }
  return num;
}

int dCollideCylCyl (dxGeom *o1, dxGeom *o2, int flags,
		dContactGeom *contact, int skip)
{
  dVector3 normal;
  dReal depth;
  int code;
dReal cylRadius1,cylRadius2;
dReal cylLength1,cylLength2;
dGeomCylinderGetParams(o1,&cylRadius1,&cylLength1);
dGeomCylinderGetParams(o2,&cylRadius2,&cylLength2);
int num = dCylCyl (dGeomGetPosition(o1),dGeomGetRotation(o1),cylRadius1,cylLength1,
				   dGeomGetPosition(o2),dGeomGetRotation(o2),cylRadius2,cylLength2,
				     normal,&depth,&code,flags & NUMC_MASK,contact,skip);

  for (int i=0; i<num; ++i) {
    CONTACT(contact,i*skip)->normal[0] = -normal[0];
    CONTACT(contact,i*skip)->normal[1] = -normal[1];
    CONTACT(contact,i*skip)->normal[2] = -normal[2];
    CONTACT(contact,i*skip)->g1 = const_cast<dxGeom*> (o1);
    CONTACT(contact,i*skip)->g2 = const_cast<dxGeom*> (o2);
  }
  return num;
}

struct dxPlane {
  dReal p[4];
};


int dCollideCylPlane 
	(
	dxGeom *o1, dxGeom *o2, int flags,
			  dContactGeom *contact, int skip){
  VERIFY (skip >= (int)sizeof(dContactGeom));
  VERIFY (dGeomGetClass(o1) == dCylinderClassUser);
  VERIFY (dGeomGetClass(o2) == dPlaneClass);
  contact->g1 = const_cast<dxGeom*> (o1);
  contact->g2 = const_cast<dxGeom*> (o2);
  
 unsigned int ret = 0;

 dReal radius;
 dReal hlz;
 dGeomCylinderGetParams(o1,&radius,&hlz);
 hlz/=REAL(2.);
 const dReal *R	=	dGeomGetRotation(o1);// rotation of cylinder
 const dReal* p	=	dGeomGetPosition(o1);
 dVector4 n;		// normal vector3
 dReal pp;
 dGeomPlaneGetParams (o2, n);
 pp=n[3];
 dReal cos1,sin1;
  cos1=dFabs(dDOT14(n,R+1));

cos1=cos1<REAL(1.) ? cos1 : REAL(1.); //cos1 may slightly exeed 1.f
sin1=dSqrt(REAL(1.)-cos1*cos1);
//////////////////////////////

dReal sidePr=cos1*hlz+sin1*radius;

dReal dist=-pp+dDOT(n,p);
dReal outDepth=sidePr-dist;

if(outDepth<0.f) return 0;

dVector3 pos;


/////////////////////////////////////////// from geom.cpp dCollideBP
  dReal Q1 = dDOT14(n,R+0);
  dReal Q2 = dDOT14(n,R+1);
  dReal Q3 = dDOT14(n,R+2);
  dReal factor =dSqrt(Q1*Q1+Q3*Q3);
  factor= factor ? factor :1.f;
  dReal A1 = radius *		Q1/factor;
  dReal A2 = hlz*Q2;
  dReal A3 = radius *		Q3/factor;

  pos[0]=p[0];
  pos[1]=p[1];
  pos[2]=p[2];

  pos[0]-= A1*R[0];
  pos[1]-= A1*R[4];
  pos[2]-= A1*R[8];

  pos[0]-= A3*R[2];
  pos[1]-= A3*R[6];
  pos[2]-= A3*R[10];

  pos[0]-= A2>0 ? hlz*R[1]:-hlz*R[1];
  pos[1]-= A2>0 ? hlz*R[5]:-hlz*R[5];
  pos[2]-= A2>0 ? hlz*R[9]:-hlz*R[9];
  
 

  contact->pos[0] = pos[0];
  contact->pos[1] = pos[1];
  contact->pos[2] = pos[2];
   contact->depth = outDepth;
  ret=1;
 
if(dFabs(Q2)>M_SQRT1_2){

  CONTACT(contact,ret*skip)->pos[0]=pos[0]+2.f*A1*R[0];
  CONTACT(contact,ret*skip)->pos[1]=pos[1]+2.f*A1*R[4];
  CONTACT(contact,ret*skip)->pos[2]=pos[2]+2.f*A1*R[8];
  CONTACT(contact,ret*skip)->depth=outDepth-dFabs(Q1*2.f*A1);

  if(CONTACT(contact,ret*skip)->depth>0.f)
  ++ret;
  
  
  CONTACT(contact,ret*skip)->pos[0]=pos[0]+2.f*A3*R[2];
  CONTACT(contact,ret*skip)->pos[1]=pos[1]+2.f*A3*R[6];
  CONTACT(contact,ret*skip)->pos[2]=pos[2]+2.f*A3*R[10];
  CONTACT(contact,ret*skip)->depth=outDepth-dFabs(Q3*2.f*A3);

  if(CONTACT(contact,ret*skip)->depth>0.f) ++ret;
} else {

  CONTACT(contact,ret*skip)->pos[0]=pos[0]+2.f*(A2>0 ? hlz*R[1]:-hlz*R[1]);
  CONTACT(contact,ret*skip)->pos[1]=pos[1]+2.f*(A2>0 ? hlz*R[5]:-hlz*R[5]);
  CONTACT(contact,ret*skip)->pos[2]=pos[2]+2.f*(A2>0 ? hlz*R[9]:-hlz*R[9]);
  CONTACT(contact,ret*skip)->depth=outDepth-dFabs(Q2*2.f*A2);

  if(CONTACT(contact,ret*skip)->depth>0.f) ++ret;
}



 for (unsigned int i=0; i<ret; ++i) {
    CONTACT(contact,i*skip)->g1 = const_cast<dxGeom*> (o1);
    CONTACT(contact,i*skip)->g2 = const_cast<dxGeom*> (o2);
	CONTACT(contact,i*skip)->normal[0] =n[0];
	CONTACT(contact,i*skip)->normal[1] =n[1];
	CONTACT(contact,i*skip)->normal[2] =n[2];
  }
  return ret;  
}

int dCollideCylRay(dxGeom *o1, dxGeom *o2, int flags,
				   dContactGeom *contact, int skip) {
					   VERIFY (skip >= (int)sizeof(dContactGeom));
					   VERIFY (dGeomGetClass(o1) == dCylinderClassUser);
					   VERIFY (dGeomGetClass(o2) == dRayClass);
					   contact->g1 = const_cast<dxGeom*> (o1);
					   contact->g2 = const_cast<dxGeom*> (o2);
					   dReal radius;
					   dReal lz;
					   dGeomCylinderGetParams(o1,&radius,&lz);
					   dReal lz2=lz*REAL(0.5);
					   const dReal *R = dGeomGetRotation(o1); // rotation of the cylinder
					   const dReal *p = dGeomGetPosition(o1); // position of the cylinder
					   dVector3 start,dir;
					   dGeomRayGet(o2,start,dir); // position and orientation of the ray
					   dReal length = dGeomRayGetLength(o2);

					   // compute some useful info
					   dVector3 cs,q,r;
					   dReal C,k;
					   cs[0] = start[0] - p[0];
					   cs[1] = start[1] - p[1];
					   cs[2] = start[2] - p[2];
					   k = dDOT41(R+1,cs);	// position of ray start along cyl axis (Y)
					   q[0] = k*R[0*4+1] - cs[0];
					   q[1] = k*R[1*4+1] - cs[1];
					   q[2] = k*R[2*4+1] - cs[2];
					   C = dDOT(q,q) - radius*radius;
					   // if C < 0 then ray start position within infinite extension of cylinder
					   // if ray start position is inside the cylinder
					   int inside_cyl=0;
					   if (C<0 && !(k<-lz2 || k>lz2)) inside_cyl=1;
					   // compute ray collision with infinite cylinder, except for the case where
					   // the ray is outside the cylinder but within the infinite cylinder
					   // (it that case the ray can only hit endcaps)
					   if (!inside_cyl && C < 0) {
						   // set k to cap position to check
						   if (k < 0) k = -lz2; else k = lz2;
					   }
					   else {
						   dReal uv = dDOT41(R+1,dir);
						   r[0] = uv*R[0*4+1] - dir[0];
						   r[1] = uv*R[1*4+1] - dir[1];
						   r[2] = uv*R[2*4+1] - dir[2];
						   dReal A = dDOT(r,r);
						   dReal B = 2*dDOT(q,r);
						   k = B*B-4*A*C;
						   if (k < 0) {
							   // the ray does not intersect the infinite cylinder, but if the ray is
							   // inside and parallel to the cylinder axis it may intersect the end
							   // caps. set k to cap position to check.
							   if (!inside_cyl) return 0;
							   if (uv < 0) k = -lz2; else k = lz2;
						   }
						   else {
							   k = dSqrt(k);
							   A = dRecip (2*A);
							   dReal alpha = (-B-k)*A;
							   if (alpha < 0) {
								   alpha = (-B+k)*A;
								   if (alpha<0) return 0;
							   }
							   if (alpha>length) return 0;
							   // the ray intersects the infinite cylinder. check to see if the
							   // intersection point is between the caps
							   contact->pos[0] = start[0] + alpha*dir[0];
							   contact->pos[1] = start[1] + alpha*dir[1];
							   contact->pos[2] = start[2] + alpha*dir[2];
							   q[0] = contact->pos[0] - p[0];
							   q[1] = contact->pos[1] - p[1];
							   q[2] = contact->pos[2] - p[2];
							   k = dDOT14(q,R+1);
							   dReal nsign = inside_cyl ? -REAL(1.) :REAL(1.);
							   if (k >= -lz2 && k <= lz2) {
								   contact->normal[0] = nsign * (contact->pos[0] -
									   (p[0] + k*R[0*4+1]));
								   contact->normal[1] = nsign * (contact->pos[1] -
									   (p[1] + k*R[1*4+1]));
								   contact->normal[2] = nsign * (contact->pos[2] -
									   (p[2] + k*R[2*4+1]));
								   dNormalize3 (contact->normal);
								   contact->depth = alpha;
								   return 1;
							   }
							   // the infinite cylinder intersection point is not between the caps.
							   // set k to cap position to check.
							   if (k < 0) k = -lz2; else k = lz2;
						   }
					   }
					   // check for ray intersection with the caps. k must indicate the cap
					   // position to check
					   // perform a ray plan interesection
					   // R+1 is the plan normal
					   q[0] = start[0] - (p[0] + k*R[0*4+1]);
					   q[1] = start[1] - (p[1] + k*R[1*4+1]);
					   q[2] = start[2] - (p[2] + k*R[2*4+1]);
					   dReal alpha = -dDOT14(q,R+1);
					   dReal k2 = dDOT14(dir,R+1);
					   if (k2==0) return 0; // ray parallel to the plane
					   alpha/=k2;
					   if (alpha<0 || alpha>length) return 0; // too short
					   contact->pos[0]=start[0]+alpha*dir[0];
					   contact->pos[1]=start[1]+alpha*dir[1];
					   contact->pos[2]=start[2]+alpha*dir[2];
					   dReal nsign = (k<0)?-REAL(1.):REAL(1.);
					   contact->normal[0]=nsign*R[0*4+1];
					   contact->normal[1]=nsign*R[1*4+1];
					   contact->normal[2]=nsign*R[2*4+1];
					   contact->depth=alpha;
					   return 1;
				   }

static  dColliderFn * dCylinderColliderFn (int num)
{
  if (num == dBoxClass) return (dColliderFn *) &dCollideCylB;
  if (num == dSphereClass) return (dColliderFn *) &dCollideCylS;
  if (num == dCylinderClassUser) return (dColliderFn *) &dCollideCylCyl;
  if (num == dPlaneClass) return (dColliderFn *) &dCollideCylPlane;
  return 0;
}


static  void dCylinderAABB (dxGeom *geom, dReal aabb[6])
{
  dReal radius,lz;
  dGeomCylinderGetParams(geom,&radius,&lz);
const dReal* R= dGeomGetRotation(geom);
const dReal* pos= dGeomGetPosition(geom);

	
  dReal xrange = REAL(0.5) * dFabs (R[1] * lz) + (dSqrt(R[0]*R[0]+R[2]*R[2]) * radius);

  dReal yrange = REAL(0.5) * dFabs (R[5] * lz) + (dSqrt(R[4]*R[4]+R[6]*R[6]) * radius);

  dReal zrange = REAL(0.5) * dFabs (R[9] * lz) + (dSqrt(R[8]*R[8]+R[10]*R[10]) * radius);

  aabb[0] = pos[0] - xrange;
  aabb[1] = pos[0] + xrange;
  aabb[2] = pos[1] - yrange;
  aabb[3] = pos[1] + yrange;
  aabb[4] = pos[2] - zrange;
  aabb[5] = pos[2] + zrange;
}

dxGeom *dCreateCylinder (dSpaceID space, dReal r, dReal lz)
{
 VERIFY (r > 0 && lz > 0);
 if (dCylinderClassUser == -1)
  {
    dGeomClass c;
    c.bytes = sizeof (dxCylinder);
    c.collider = &dCylinderColliderFn;
    c.aabb = &dCylinderAABB;
    c.aabb_test = 0;
    c.dtor = 0;
    dCylinderClassUser=dCreateGeomClass (&c);

  }

  dGeomID g = dCreateGeom (dCylinderClassUser);
  if (space) dSpaceAdd (space,g);
  dxCylinder *c = (dxCylinder*) dGeomGetClassData(g);

  c->radius = r;
  c->lz = lz;
  return g;
}



void dGeomCylinderSetParams (dGeomID g, dReal radius, dReal length)
{
  VERIFY2 (g && dGeomGetClass(g) == dCylinderClassUser ,"argument not a cylinder");
  VERIFY (radius > 0 && length > 0);
  dxCylinder *c = (dxCylinder*) dGeomGetClassData(g);
  c->radius = radius;
  c->lz = length;
}



void dGeomCylinderGetParams (dGeomID g, dReal *radius, dReal *length)
{
  VERIFY2 (g && dGeomGetClass(g) == dCylinderClassUser ,"argument not a cylinder");
  dxCylinder *c = (dxCylinder*) dGeomGetClassData(g);
  *radius = c->radius;
  *length = c->lz;
}

