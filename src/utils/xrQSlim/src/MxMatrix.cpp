/************************************************************************

  NxN Matrix class

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: MxMatrix.cxx,v 1.4 1998/10/26 21:09:09 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "MxMatrix.h"

// Originally from Paul's matrix library.
//
// Returns determinant of a, and b=a inverse.
// If matrix is singular, returns 0 and leaves trash in b.
//
// Uses Gaussian elimination with partial pivoting.

#define SWAP(a, b, t)   {t = a; a = b; b = t;}
static double internal_invert(MxMatrix& A, MxMatrix& B)
{
    unsigned int N = A.dim();
    unsigned int i, j=0, k;
    double max, t, det, pivot;

    /*---------- forward elimination ----------*/

    for (i=0; i<N; i++)                 /* put identity matrix in B */
        for (j=0; j<N; j++)
            B(i, j) = (double)(i==j);

    det = 1.0;
    for (i=0; i<N; i++) {               /* eliminate in column i, below diag */
        max = -1.;
        for (k=i; k<N; k++)             /* find pivot for column i */
            if (_abs(A(k, i)) > max) {
                max = _abs(A(k, i));
                j = k;
            }
        if (max<=0.) return 0.;         /* if no nonzero pivot, PUNT */
        if (j!=i) {                     /* swap rows i and j */
            for (k=i; k<N; k++)
                SWAP(A(i, k), A(j, k), t);
            for (k=0; k<N; k++)
                SWAP(B(i, k), B(j, k), t);
            det = -det;
        }
        pivot = A(i, i);
        det *= pivot;
        for (k=i+1; k<N; k++)           /* only do elems to right of pivot */
            A(i, k) /= pivot;
        for (k=0; k<N; k++)
            B(i, k) /= pivot;
        /* we know that A(i, i) will be set to 1, so don't bother to do it */

        for (j=i+1; j<N; j++) {         /* eliminate in rows below i */
            t = A(j, i);                /* we're gonna zero this guy */
            for (k=i+1; k<N; k++)       /* subtract scaled row i from row j */
                A(j, k) -= A(i, k)*t;   /* (ignore k<=i, we know they're 0) */
            for (k=0; k<N; k++)
                B(j, k) -= B(i, k)*t;
        }
    }

    /*---------- backward elimination ----------*/

    for (i=N-1; i>0; i--) {             /* eliminate in column i, above diag */
        for (j=0; j<i; j++) {           /* eliminate in rows above i */
            t = A(j, i);                /* we're gonna zero this guy */
            for (k=0; k<N; k++)         /* subtract scaled row i from row j */
                B(j, k) -= B(i, k)*t;
        }
    }

    return det;
}

#undef SWAP


double mxm_invert(MxMatrix& r, const MxMatrix& a)
{
    MxMatrix a2(a);
    return internal_invert(a2, r);
}
