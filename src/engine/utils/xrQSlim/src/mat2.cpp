/************************************************************************

  2x2 Matrix class

  $Id: mat2.cxx,v 1.4 2001/02/08 21:28:53 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "MxDefines.h"
#include "mat2.h"

Mat2 Mat2::I() { return Mat2(1,0,  0,1); }

Mat2 &Mat2::diag(double d)
{
    row[0][0] = d;   row[0][1] = 0;
    row[1][0] = 0;   row[1][1] = d;

    return *this;
}

Mat2 operator*(const Mat2 &n, const Mat2& m)
{
    Mat2 A;
    int i,j;

    for(i=0;i<2;i++)
	for(j=0;j<2;j++)
	    A(i,j) = n[i]*m.col(j);

    return A;
}

double invert(Mat2 &inv, const Mat2 &m)
{
    double d = det(m);

    if( d==0.0 )
	return 0.0;

    inv(0, 0) =  m(1,1)/d;
    inv(0, 1) = -m(0,1)/d;
    inv(1, 0) = -m(1,0)/d;
    inv(1, 1) =  m(0,0)/d;

    return d;
}

bool eigenvalues(const Mat2& M, Vec2& evals)
{
    double B = -M(0,0)-M(1,1);
    double C = det(M);

    double dis = B*B - 4.0*C;
    if( dis<FEQ_EPS )
	return false;
    else
    {
	double s = _sqrt(dis);

	evals[0] = 0.5*(-B + s);
	evals[1] = 0.5*(-B - s);
	return true;
    }
}

bool eigenvectors(const Mat2& M, const Vec2& evals, Vec2 evecs[2])
{
    evecs[0] = Vec2(-M(0,1), M(0,0)-evals[0]);
    evecs[1] = Vec2(-M(0,1), M(0,0)-evals[1]);

    unitize(evecs[0]);
    unitize(evecs[1]);

    return true;
}

bool eigen(const Mat2& M, Vec2& evals, Vec2 evecs[2])
{
    bool result = eigenvalues(M, evals);
    if( result )
	eigenvectors(M, evals, evecs);
    return result;
}

