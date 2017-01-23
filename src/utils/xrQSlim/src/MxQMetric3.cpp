/************************************************************************

  3D Quadric Error Metrics

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: MxQMetric3.cxx,v 1.13.2.1 2002/01/31 19:29:48 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "MxDefines.h"
#include "MxQMetric3.h"

#include "MxMat2.h"

void MxQuadric3::init(double a, double b, double c, double d, double area)
{
    a2 = a*a;  ab = a*b;  ac = a*c;  ad = a*d;
               b2 = b*b;  bc = b*c;  bd = b*d;
	                  c2 = c*c;  cd = c*d;
			             d2 = d*d;

    r = area;
}

void MxQuadric3::init(const Mat4& Q, double area)
{
    a2 = Q(0,0);  ab = Q(0,1);  ac = Q(0,2);  ad = Q(0,3);
                  b2 = Q(1,1);  bc = Q(1,2);  bd = Q(1,3);
		                c2 = Q(2,2);  cd = Q(2,3);
				              d2 = Q(3,3);

    r = area;
}

Mat3 MxQuadric3::tensor() const
{
    return Mat3(Vec3(a2, ab, ac),
		Vec3(ab, b2, bc),
		Vec3(ac, bc, c2));
}

Mat4 MxQuadric3::homogeneous() const
{
    return Mat4(Vec4(a2, ab, ac, ad),
		Vec4(ab, b2, bc, bd),
		Vec4(ac, bc, c2, cd),
		Vec4(ad, bd, cd, d2));
}

void MxQuadric3::set_coefficients(const double *v)
{
    a2 = v[0];  ab = v[1];  ac = v[2];  ad = v[3];
                b2 = v[4];  bc = v[5];  bd = v[6];
		            c2 = v[7];  cd = v[8];
                                        d2 = v[9];
}

void MxQuadric3::point_constraint(const float *p)
{
    // A point constraint quadric measures the squared distance
    // of any point v to the given point p.

    a2=b2=c2=1.0;  ab=ac=bc= 0.0;		// A = I
    ad=-p[0]; bd=-p[1]; cd=-p[2];		// b = -p
    d2 = p[0]*p[0] + p[1]*p[1] + p[2]*p[2];	// c = p*p
}

MxQuadric3& MxQuadric3::operator=(const MxQuadric3& Q)
{
    r = Q.r;

    a2 = Q.a2;  ab = Q.ab;  ac = Q.ac;  ad = Q.ad;
                b2 = Q.b2;  bc = Q.bc;  bd = Q.bd;
		            c2 = Q.c2;  cd = Q.cd;
                                        d2 = Q.d2;

    return *this;
}

MxQuadric3& MxQuadric3::operator+=(const MxQuadric3& Q)
{
    // Accumulate area
    r += Q.r;

    // Accumulate coefficients
    a2 += Q.a2;  ab += Q.ab;  ac += Q.ac;  ad += Q.ad;
                 b2 += Q.b2;  bc += Q.bc;  bd += Q.bd;
		              c2 += Q.c2;  cd += Q.cd;
			                   d2 += Q.d2;

    return *this;
}

MxQuadric3& MxQuadric3::operator-=(const MxQuadric3& Q)
{
    r -= Q.r;

    a2 -= Q.a2;  ab -= Q.ab;  ac -= Q.ac;  ad -= Q.ad;
                 b2 -= Q.b2;  bc -= Q.bc;  bd -= Q.bd;
		              c2 -= Q.c2;  cd -= Q.cd;
			                   d2 -= Q.d2;

    return *this;
}

MxQuadric3& MxQuadric3::operator*=(double s)
{
    // Scale coefficients
    a2 *= s;  ab *= s;  ac *= s;  ad *= s;
              b2 *= s;  bc *= s;  bd *= s;
                        c2 *= s;  cd *= s;
			          d2 *= s;

    return *this;
}

MxQuadric3& MxQuadric3::transform(const Mat4& P)
{
    Mat4 Q = homogeneous();
    Mat4 Pa = adjoint(P);

    // Compute:  trans(Pa) * Q * Pa
    // NOTE: Pa is symmetric since Q is symmetric

    Q = Pa * Q * Pa;


    // ??BUG: Should we be transforming the area??
    init(Q, r);

    return *this;
}


double MxQuadric3::evaluate(double x, double y, double z) const
{
    // Evaluate vAv + 2bv + c

    return x*x*a2 + 2*x*y*ab + 2*x*z*ac + 2*x*ad
	          + y*y*b2   + 2*y*z*bc + 2*y*bd
	                     + z*z*c2   + 2*z*cd
	                                + d2;
}

bool MxQuadric3::optimize(Vec3& v) const
{
    Mat3 Ainv;
    double det = invert(Ainv, tensor());
    if( FEQ(det, 0.0, 1e-12) )
	return false;

    v = -(Ainv*vector());

    return true;
}

bool MxQuadric3::optimize(float *x, float *y, float *z) const
{
    Vec3 v;

    bool success = optimize(v);
    if( success )
    {
	*x = (float)v[0];
	*y = (float)v[1];
	*z = (float)v[2];
    }
    return success;
}

bool MxQuadric3::optimize(Vec3& v, const Vec3& v1, const Vec3& v2) const
{
    Vec3 d = v1 - v2;
    Mat3 A = tensor();

    Vec3 Av2 = A*v2;
    Vec3 Ad  = A*d;

    double denom = 2.0*d*Ad;
    if( FEQ(denom, 0.0, 1e-12) )
	return false;

    double a =  ( -2*(vector()*d) - (d*Av2) - (v2*Ad) ) / ( 2*(d*Ad) );

    if( a<0.0 ) a=0.0; else if( a>1.0 ) a=1.0;

    v = a*d + v2;
    return true;
}


bool MxQuadric3::optimize(Vec3& v, const Vec3& v1,
			  const Vec3& v2, const Vec3& v3) const
{
    Vec3 d13 = v1 - v3;
    Vec3 d23 = v2 - v3;
    Mat3 A = tensor();
    Vec3 B = vector();

    Vec3 Ad13 = A*d13;
    Vec3 Ad23 = A*d23;
    Vec3 Av3  = A*v3;

    double d13_d23 = (d13*Ad23) + (d23*Ad13);
    double v3_d13  = (d13*Av3) + (v3*Ad13);
    double v3_d23  = (d23*Av3) + (v3*Ad23);

    double d23Ad23 = d23*Ad23;
    double d13Ad13 = d13*Ad13;

    double denom = d13Ad13*d23Ad23 - 2*d13_d23;
    if( FEQ(denom, 0.0, 1e-12) )
	return false;

    double a = ( d23Ad23*(2*(B*d13) + v3_d13) -
		  d13_d23*(2*(B*d23) + v3_d23) ) / -denom;

    double b = ( d13Ad13*(2*(B*d23) + v3_d23) -
		  d13_d23*(2*(B*d13) + v3_d13) ) / -denom;

    if( a<0.0 ) a=0.0; else if( a>1.0 ) a=1.0;
    if( b<0.0 ) b=0.0; else if( b>1.0 ) b=1.0;

    v = a*d13 + b*d23 + v3;
    return true;
}
