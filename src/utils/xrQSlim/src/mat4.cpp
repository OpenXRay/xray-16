/************************************************************************

  4x4 Matrix class
  
  $Id: mat4.cxx,v 1.4 2000/12/04 06:18:37 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "mat4.h"

Mat4 Mat4::I()
{
    return Mat4(Vec4(1,0,0,0),Vec4(0,1,0,0),Vec4(0,0,1,0),Vec4(0,0,0,1));
}

Mat4 translation_matrix(const Vec3& d)
{
    return Mat4(Vec4(1, 0, 0, d[0]),
		Vec4(0, 1, 0, d[1]),
		Vec4(0, 0, 1, d[2]),
		Vec4(0, 0, 0, 1));
}

Mat4 scaling_matrix(const Vec3& s)
{
    return Mat4(Vec4(s[0], 0,    0,    0),
		Vec4(0,    s[1], 0,    0),
		Vec4(0,    0,    s[2], 0),
		Vec4(0,    0,    0,    1));
}

Mat4 rotation_matrix_rad(double theta, const Vec3& axis)
{
    double c=_cos(theta), s=_sin(theta),
	xx=axis[0]*axis[0],  yy=axis[1]*axis[1],  zz=axis[2]*axis[2],
	xy=axis[0]*axis[1],  yz=axis[1]*axis[2],  xz=axis[0]*axis[2];

    double xs=axis[0]*s, ys=axis[1]*s, zs=axis[2]*s;

    Mat4 M;
    M(0,0)=xx*(1-c)+c;  M(0,1)=xy*(1-c)-zs;  M(0,2)=xz*(1-c)+ys;  M(0,3) = 0;
    M(1,0)=xy*(1-c)+zs;  M(1,1)=yy*(1-c)+c;  M(1,2)=yz*(1-c)-xs;  M(1,3)=0;
    M(2,0)=xz*(1-c)-ys;  M(2,1)=yz*(1-c)+xs;  M(2,2)=zz*(1-c)+c;  M(2,3)=0;
    M(3,0)=0;  M(3,1)=0;  M(3,2)=0;  M(3,3)=1;

    return M;
}

Mat4 perspective_matrix(double fovy, double aspect, double zmin, double zmax)
{
    double A, B;
    Mat4 M;

    if( zmax==0.0 )
    {
	A = B = 1.0;
    }
    else
    {
	A = (zmax+zmin)/(zmin-zmax);
	B = (2*zmax*zmin)/(zmin-zmax);
    }

    double f = 1.0/tan(fovy*M_PI/180.0/2.0);
    M(0,0) = f/aspect;
    M(1,1) = f;
    M(2,2) = A;
    M(2,3) = B;
    M(3,2) = -1;
    M(3,3) = 0;

    return M;
}

Mat4 lookat_matrix(const Vec3& from, const Vec3& at, const Vec3& v_up)
{
    Vec3 up = v_up;       unitize(up);
    Vec3 f = at - from;   unitize(f);

    Vec3 s=f^up;
    Vec3 u=s^f;

    // NOTE: These steps are left out of the GL man page!!
    unitize(s);
    unitize(u);

    Mat4 M(Vec4(s, 0), Vec4(u, 0), Vec4(-f, 0), Vec4(0, 0, 0, 1));

    return M * translation_matrix(-from);
}

Mat4 viewport_matrix(double w, double h)
{
    return scaling_matrix(Vec3(w/2.0, -h/2.0, 1)) *
	translation_matrix(Vec3(1, -1, 0));
}

Mat4 operator*(const Mat4& n, const Mat4& m)
{
    Mat4 A;
    int i,j;

    for(i=0;i<4;i++)
	for(j=0;j<4;j++)
	    A(i,j) = n[i]*m.col(j);

    return A;
}

Mat4 adjoint(const Mat4& m)
{
    Mat4 A;

    A[0] = cross( m[1], m[2], m[3]);
    A[1] = cross(-m[0], m[2], m[3]);
    A[2] = cross( m[0], m[1], m[3]);
    A[3] = cross(-m[0], m[1], m[2]);
        
    return A;
}

double invert_cramer(Mat4& inv, const Mat4& m)
{
    Mat4 A = adjoint(m);
    double d = A[0] * m[0];

    if( d==0.0 )
	return 0.0;

    inv = transpose(A) / d;
    return d;
}



// Matrix inversion code for 4x4 matrices using Gaussian elimination
// with partial pivoting.  This is a specialized version of a
// procedure originally due to Paul Heckbert <ph@cs.cmu.edu>.
//
// Returns determinant of A, and B=inverse(A)
// If matrix A is singular, returns 0 and leaves trash in B.
//
#define SWAP(a, b, t)   {t = a; a = b; b = t;}
double invert(Mat4& B, const Mat4& m)
{
    Mat4 A = m;
    int i, j=0, k;
    double max, t, det, pivot;

    /*---------- forward elimination ----------*/

    for (i=0; i<4; i++)                 /* put identity matrix in B */
        for (j=0; j<4; j++)
            B(i, j) = (double)(i==j);

    det = 1.0;
    for (i=0; i<4; i++) {               /* eliminate in column i, below diag */
        max = -1.;
        for (k=i; k<4; k++)             /* find pivot for column i */
            if (_abs(A(k, i)) > max) {
                max = _abs(A(k, i));
                j = k;
            }
        if (max<=0.) return 0.;         /* if no nonzero pivot, PUNT */
        if (j!=i) {                     /* swap rows i and j */
            for (k=i; k<4; k++)
                SWAP(A(i, k), A(j, k), t);
            for (k=0; k<4; k++)
                SWAP(B(i, k), B(j, k), t);
            det = -det;
        }
        pivot = A(i, i);
        det *= pivot;
        for (k=i+1; k<4; k++)           /* only do elems to right of pivot */
            A(i, k) /= pivot;
        for (k=0; k<4; k++)
            B(i, k) /= pivot;
        /* we know that A(i, i) will be set to 1, so don't bother to do it */

        for (j=i+1; j<4; j++) {         /* eliminate in rows below i */
            t = A(j, i);                /* we're gonna zero this guy */
            for (k=i+1; k<4; k++)       /* subtract scaled row i from row j */
                A(j, k) -= A(i, k)*t;   /* (ignore k<=i, we know they're 0) */
            for (k=0; k<4; k++)
                B(j, k) -= B(i, k)*t;
        }
    }

    /*---------- backward elimination ----------*/

    for (i=4-1; i>0; i--) {             /* eliminate in column i, above diag */
        for (j=0; j<i; j++) {           /* eliminate in rows above i */
            t = A(j, i);                /* we're gonna zero this guy */
            for (k=0; k<4; k++)         /* subtract scaled row i from row j */
                B(j, k) -= B(i, k)*t;
        }
    }

    return det;
}
