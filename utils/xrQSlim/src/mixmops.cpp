/************************************************************************

  NxN Matrix class

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: mixmops.cxx,v 1.7 1998/10/26 21:09:38 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "MxMatrix.h"

// This section originally from Paul's matrix library.

#define SWAP(a, b, t)   {t = a; a = b; b = t;}
#define A(i,j) mxm_ref(_a, i, j, N)
#define B(i,j) mxm_ref(_b, i, j, N)

// Solve nxn system Ax=b.
// Leaves solution x in b, and destroys original A and b vectors.
// Return value is determinant of A.
// If system is singular, returns 0 and leaves trash in b
//
// Uses Gaussian elimination with partial pivoting.
//
static
double internal_solve(double *_a, double *b, const int N)
{
    int i, j=0, k;
    double max, t, det, sum, pivot;

    /*---------- forward elimination ----------*/

    det = 1.0;
    for (i=0; i<N; i++) {		/* eliminate in column i */
	max = -1.0;
	for (k=i; k<N; k++)		/* find pivot for column i */
	    if (_abs(A(k, i)) > max) {
		max = _abs(A(k, i));
		j = k;
	    }
	if (max<=0.) return 0.0;	/* if no nonzero pivot, PUNT */
	if (j!=i) {			/* swap rows i and j */
	    for (k=i; k<N; k++)
		SWAP(A(i, k), A(j, k), t);
	    det = -det;
	    SWAP(b[i], b[j], t);	/* swap elements of column vector */
	}
	pivot = A(i, i);
	det *= pivot;
	for (k=i+1; k<N; k++)		/* only do elems to right of pivot */
	    A(i, k) /= pivot;

	/* we know that A(i, i) will be set to 1, so don't bother to do it */
	b[i] /= pivot;
	for (j=i+1; j<N; j++) {		/* eliminate in rows below i */
	    t = A(j, i);		/* we're gonna zero this guy */
	    for (k=i+1; k<N; k++)	/* subtract scaled row i from row j */
		A(j, k) -= A(i, k)*t;	/* (ignore k<=i, we know they're 0) */
	    b[j] -= b[i]*t;
	}
    }

    /*---------- back substitution ----------*/

    for (i=N-1; i>=0; i--) {		/* solve for x[i] (put it in b[i]) */
	sum = b[i];
	for (k=i+1; k<N; k++)		/* really A(i, k)*x[k] */
	    sum -= A(i, k)*b[k];
	b[i] = sum;
    }

    return det;
}

// Returns determinant of a, and b=a inverse.
// If matrix is singular, returns 0 and leaves trash in b.
//
// Uses Gaussian elimination with partial pivoting.
//
static
double internal_invert(double *_a, double *_b, const int N)
{
    unsigned int i, j=0, k;
    double max, t, det, pivot;

    /*---------- forward elimination ----------*/

    for (i=0; i<(unsigned int)N; i++)                 /* put identity matrix in B */
        for (j=0; j<(unsigned int)N; j++)
            B(i, j) = (double)(i==j);

    det = 1.0;
    for (i=0; i<(unsigned int)N; i++) {               /* eliminate in column i, below diag */
        max = -1.;
        for (k=i; k<(unsigned int)N; k++)             /* find pivot for column i */
            if (_abs(A(k, i)) > max) {
                max = _abs(A(k, i));
                j = k;
            }
        if (max<=0.) return 0.;         /* if no nonzero pivot, PUNT */
        if (j!=i) {                     /* swap rows i and j */
            for (k=i; k<(unsigned int)N; k++)
                SWAP(A(i, k), A(j, k), t);
            for (k=0; k<(unsigned int)N; k++)
                SWAP(B(i, k), B(j, k), t);
            det = -det;
        }
        pivot = A(i, i);
        det *= pivot;
        for (k=i+1; k<(unsigned int)N; k++)           /* only do elems to right of pivot */
            A(i, k) /= pivot;
        for (k=0; k<(unsigned int)N; k++)
            B(i, k) /= pivot;
        /* we know that A(i, i) will be set to 1, so don't bother to do it */

        for (j=i+1; j<(unsigned int)N; j++) {         /* eliminate in rows below i */
            t = A(j, i);                /* we're gonna zero this guy */
            for (k=i+1; k<(unsigned int)N; k++)       /* subtract scaled row i from row j */
                A(j, k) -= A(i, k)*t;   /* (ignore k<=i, we know they're 0) */
            for (k=0; k<(unsigned int)N; k++)
                B(j, k) -= B(i, k)*t;
        }
    }

    /*---------- backward elimination ----------*/

    for (i=N-1; i>0; i--) {             /* eliminate in column i, above diag */
        for (j=0; j<i; j++) {           /* eliminate in rows above i */
            t = A(j, i);                /* we're gonna zero this guy */
            for (k=0; k<(unsigned int)N; k++)         /* subtract scaled row i from row j */
                B(j, k) -= B(i, k)*t;
        }
    }

    return det;
}

#undef A
#undef B
#undef SWAP

float mxm_invert(float *r, const float *a, const int N)
{
    mxm_local_block(a2, double, N);
    mxm_local_block(r2, double, N);

    unsigned int i;
    for(i=0; i<(unsigned int)N*N; i++) a2[i] = a[i];
    float det = (float)internal_invert(a2, r2, N);
    for(i=0; i<(unsigned int)N*N; i++) r[i] = (float)r2[i];

    mxm_free_local(a2);
    mxm_free_local(r2);
    return det;
}

double mxm_invert(double *r, const double *a, const int N)
{
    mxm_local_block(a2, double, N);
    mxm_set(a2, a, N);
    double det = internal_invert(a2, r, N);
    mxm_free_local(a2);
    return det;
}

double mxm_solve(double *x, const double *A, const double *b, const int N)
{
    mxm_local_block(a2, double, N);
    mxm_set(a2, A, N);
    mxv_set(x, b, N);
    double det = internal_solve(a2, x, N);
    mxm_free_local(a2);
    return det;
}

// Originally based on public domain code by <Ajay_Shah@rand.org>
// which can be found at http://lib.stat.cmu.edu/general/ajay
//
// The factorization is valid as long as the returned nullity == 0
// U contains the upper triangular factor itself.
//
int mxm_cholesky(double *U, const double *A, const int N)
{
    double sum;

    int nullity = 0;
    mxm_set(U, 0.0, N);

    for(int i=0; i<N; i++)
    {
	/* First compute U[i][i] */
	sum = mxm_ref(A, i, i, N);

	for(int j=0; j<=(i-1); j++)
	    sum -= mxm_ref(U, j, i, N) * mxm_ref(U, j, i, N);

	if( sum > 0 )
	{
	    mxm_ref(U, i, i, N) = _sqrt(sum);

	    /* Now find elements U[i][k], k > i. */
	    for(int k=(i+1); k<N; k++)
	    {
		sum = mxm_ref(A, i, k, N);

		for(int j=0; j<=(i-1); j++)
		    sum -= mxm_ref(U, j, i, N)*mxm_ref(U, j, k, N);
		
		mxm_ref(U, i, k, N) = sum / mxm_ref(U, i, i, N);
	    }
	}
	else
	{
	    for(int k=i; k<N; k++) mxm_ref(U, i, k, N) = 0.0;
	    nullity++;
	}
    }

    return nullity;
}
