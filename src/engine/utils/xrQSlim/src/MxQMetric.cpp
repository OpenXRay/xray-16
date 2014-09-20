/************************************************************************

  n-D Quadric Error Metrics

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: MxQMetric.cxx,v 1.5 1998/10/26 21:09:17 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "MxQMetric.h"

static
void symmetric_subfrom(MxMatrix& A, const MxVector& a, const MxVector& b)
{
    for(unsigned int i=0; i<A.dim(); i++)  for(unsigned int j=0; j<A.dim(); j++)
	A(i,j) -= a[i]*b[j];
}

MxQuadric::MxQuadric(const MxVector& p1,const MxVector& p2,const MxVector& p3,
		     double area)
    : A(p1.dim()), b(p1.dim())
{
    VERIFY( p1.dim()==p2.dim() && p1.dim()==p3.dim() );

    MxVector e1=p2; e1-=p1; unitize(e1); // e1 = p2-p1; unitize
    MxVector e2=p3; e2-=p1;              // e2 = p3-p1
    MxVector t = e1;
    t*=e1*e2; e2-=t; unitize(e2);        // e2 = p3-p1-e1*(e1*(p3-p1)); unitize

    double p1e1 = p1*e1;
    double p1e2 = p1*e2;

    mxm_identity(A, A.dim());
    symmetric_subfrom(A, e1,e1);
    symmetric_subfrom(A, e2,e2);

    // b = e1*p1e1 + e2*p1e2 - p1
    b=e1; b*=p1e1; t=e2; t*=p1e2; b += t; b -= p1;

    c = p1*p1 - p1e1*p1e1 - p1e2*p1e2;

    r = area;
}

MxQuadric::MxQuadric(const MxQuadric3& Q3, unsigned int N)
    : A(N), b(N)
{
    unsigned int i, j;

    clear();

    Mat3 A3 = Q3.tensor();
    Vec3 b3 = Q3.vector();

    for(i=0; i<3; i++)
    {
	for(j=0; j<3; j++)
	    A(i,j) = A3(i,j);

	b[i] = b3[i];
    }

    c = Q3.offset();
    r = Q3.area();
}

MxMatrix& MxQuadric::homogeneous(MxMatrix& H) const
{
    VERIFY( H.dim() == A.dim()+1 );

    unsigned int i, j;

    for(i=0; i<A.dim(); i++)  for(j=0; j<A.dim(); i++)
	H(i,j) = A(i,j);

    for(i=0; i<b.dim(); i++)
	H(i, b.dim()) = H(b.dim(), i) = b[i];

    H(b.dim(), b.dim()) = c;

    return H;
}

double MxQuadric::evaluate(const MxVector& v) const
{
    VERIFY( v.dim() == b.dim() );
    return v*(A*v) + 2*(b*v) + c;
}

bool MxQuadric::optimize(MxVector& v) const
{
    MxMatrix Ainv(A.dim());

    double det = A.invert(Ainv);
    if( FEQ(det, 0.0, 1e-12) )
	return false;

    v = (Ainv * b);
    mxv_neg(v, v.dim());

    return true;
}
