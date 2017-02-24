#ifndef MIXMOPS_INCLUDED // -*- C++ -*-
#define MIXMOPS_INCLUDED //
#endif // WARNING:  Multiple inclusions allowed

/************************************************************************

  Low-level matrix math operations.

  This header file is intended for internal library use only.  You should
  understand what's going on in here before trying to use it yourself.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: mixmops.h,v 1.8 1999/11/30 02:39:38 garland Exp $

 ************************************************************************/

#ifdef MIXVOPS_DEFAULT_DIM
#define __DIM const int N = MIXVOPS_DEFAULT_DIM
#else
#define __DIM const int N
#endif

#ifndef __T
#define __T double
#endif

#define forall(i, N) for (unsigned int i = 0; i < (unsigned int)N * N; i++)

#define def3(name, op)\
    inline __T* name(__T* r, const __T* a, const __T* b, __DIM)\
    {\
        forall(i, N) op;\
        return r;\
    }

#define def2(name, op)\
    inline __T* name(__T* r, const __T* a, __DIM)\
    {\
        forall(i, N) op;\
        return r;\
    }

#define def1(name, op)\
    inline __T* name(__T* r, __DIM)\
    {\
        forall(i, N) op;\
        return r;\
    }

#define def2s(name, op)\
    inline __T* name(__T* r, const __T* a, __T d, __DIM)\
    {\
        forall(i, N) op;\
        return r;\
    }

#define def1s(name, op)\
    inline __T* name(__T* r, __T d, __DIM)\
    {\
        forall(i, N) op;\
        return r;\
    }

def3(mxm_add, r[i] = a[i] + b[i])
def3(mxm_sub, r[i] = a[i] - b[i])
def2(mxm_addinto, r[i] += a[i])
def2(mxm_subfrom, r[i] -= a[i])
def2(mxm_set, r[i] = a[i])
def1(mxm_neg, r[i] = -r[i])
def2(mxm_neg, r[i] = -a[i])
def1s(mxm_set, r[i] = d)
def1s(mxm_scale, r[i] *= d)
def1s(mxm_invscale, r[i] /= d)
def2s(mxm_scale, r[i] = a[i] * d)
def2s(mxm_invscale, r[i] = a[i] / d)

inline __T& mxm_ref(__T* A, unsigned int i, unsigned int j, __DIM)
{ return A[i * N + j]; }
inline __T mxm_ref(const __T* A, unsigned int i, unsigned int j, __DIM) { return A[i * N + j]; }
inline __T* mxm_row(__T* A, unsigned int i, __DIM) { return A + i * N; }
inline const __T* mxm_row(const __T* A, unsigned int i, __DIM) { return A + i * N; }
inline __T* mxm_identity(__T* A, __DIM)
{
    mxm_set(A, 0.0, N);
    for (unsigned int i = 0; i < (unsigned int)N; i++)
        mxm_ref(A, i, i, N) = 1.0;
    return A;
}

inline __T* mxm_xform(__T* r, const __T* A, const __T* x, __DIM)
{
    const __T* a = A;
    for (unsigned int i = 0; i < (unsigned int)N; i++)
    {
        r[i] = 0.0;
        for (unsigned int j = 0; j < (unsigned int)N; j++)
            r[i] += (*a++) * x[j];
    }
    return r;
}

////////////////////////////////////////////////////////////////////////
// The following couple of procedures are fairly straightforward but
// rather inefficient. mxm_ref() provides an easy way to access the
// (i,j) element of the matrix, but it's inefficient for looping over
// all the elements.
//
inline __T* mxm_outerprod(__T* A, const __T* u, const __T* v, __DIM)
{
    for (unsigned int i = 0; i < (unsigned int)N; i++)
        for (unsigned int j = 0; j < (unsigned int)N; j++)
            mxm_ref(A, i, j, N) = u[i] * v[j];
    return A;
}

inline __T* mxm_mul(__T* r, const __T* a, const __T* b, __DIM)
{
    mxm_set(r, 0.0, N);

    for (unsigned int i = 0; i < (unsigned int)N; i++)
        for (unsigned int j = 0; j < (unsigned int)N; j++)
        {
            for (unsigned int k = 0; k < (unsigned int)N; k++)
                mxm_ref(r, i, j, N) += mxm_ref(a, i, k, N) * mxm_ref(b, k, j, N);
        }

    return r;
}

//
// Only actually implemented for double (and maybe float).
//
extern __T mxm_invert(__T* r, const __T* a, __DIM);
extern __T mxm_solve(__T* x, const __T* A, const __T* b, __DIM);
extern int mxm_cholesky(__T* c, const __T* a, __DIM);
/*
inline ostream& mxm_write(ostream& out, const __T *a, __DIM)
{
    for(unsigned int i=0; i<N; i++)
    {
    for(unsigned int j=0; j<N; j++)
        out << mxm_ref(a, i, j, N) << " ";
    out << endl;
    }

    return out;
}
inline ostream& mxm_write(const __T *a, __DIM) {return mxm_write(cout, a, N);}
*/
#ifndef mxm_local_block
#ifdef __GNUC__
#define mxm_local_block(a, T, N) T a[N * N]
#define mxm_free_local(a)
#else
#ifdef HAVE_ALLOCA
#include <alloca.h>
#define mxm_local_block(a, T, N) T* a = (T*)alloca(sizeof(T) * (N) * (N))
#define mxm_free_local(a)
#else
#define mxm_local_block(a, T, N) T* a = xr_alloc<T>(sizeof(T) * (N) * (N))
#define mxm_free_local(a) xr_free(a)
#endif
#endif
#endif

#undef __T
#undef __DIM
#undef forall
#undef def3
#undef def2
#undef def1
#undef def2s
#undef def1s

// MIXMOPS_INCLUDED
