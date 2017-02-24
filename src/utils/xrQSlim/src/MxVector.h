#ifndef MXVECTOR_INCLUDED // -*- C++ -*-
#define MXVECTOR_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  Generic n-dimensional vector class

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxVector.h,v 1.9 1998/10/28 16:16:06 garland Exp $

 ************************************************************************/

#ifdef __T
#undef __T
#endif

#define __T float
#include "mixvops.h"

#define __T double
#include "mixvops.h"

////////////////////////////////////////////////////////////////////////
//
// MxVBlock -- fixed-size vector class template
//
// The idea here is that we want a small collection of vector classes,
// {2,3,4}D say, and we want *zero* space overhead.  No length fields,
// no vtables, just N numerical values.
//
template <class T, unsigned int N>
class MxVBlock
{
private:
    T elt[N];

protected:
    inline void copy(const MxVBlock<T, N>& v)
    {
        for (unsigned int i = 0; i < N; i++)
            elt[i] = v.elt[i];
    }

public:
    MxVBlock() {}
    MxVBlock(const MxVBlock<T, N>& v) { mxv_set(elt, v, N); }
    MxVBlock(const T* v) { mxv_set(elt, v, N); }
    inline unsigned int dim() const { return N; }
    T& operator()(unsigned int i)
    {
        VERIFY(i < N);
        return elt[i];
    }
    T operator()(unsigned int i) const
    {
        VERIFY(i < N);
        return elt[i];
    }

    operator T*() { return elt; }
    operator const T*() const { return elt; }
    // In-place arithmetic methods
    //
    inline MxVBlock<T, N>& operator=(const MxVBlock<T, N>& v)
    {
        mxv_set(elt, v, N);
        return *this;
    }
    inline MxVBlock<T, N>& operator+=(const MxVBlock<T, N>& v)
    {
        mxv_addinto(elt, v, N);
        return *this;
    }
    inline MxVBlock<T, N>& operator-=(const MxVBlock<T, N>& v)
    {
        mxv_subfrom(elt, v, N);
        return *this;
    }
    inline MxVBlock<T, N>& operator*=(T s)
    {
        mxv_scale(elt, s, N);
        return *this;
    }
    inline MxVBlock<T, N>& operator/=(T s)
    {
        mxv_invscale(elt, s, N);
        return *this;
    }
    inline MxVBlock<T, N>& negate()
    {
        mxv_neg(elt, N);
        return *this;
    }

    // Binary arithmetic methods
    //
    inline MxVBlock<T, N> operator+(const MxVBlock<T, N>& v) const
    {
        MxVBlock<T, N> r;
        mxv_add(r, elt, v, N);
        return r;
    }
    inline MxVBlock<T, N> operator-(const MxVBlock<T, N>& v) const
    {
        MxVBlock<T, N> r;
        mxv_sub(r, elt, v, N);
        return r;
    }
    inline MxVBlock<T, N> operator*(T s) const
    {
        MxVBlock<T, N> r;
        mxv_scale(r, elt, s, N);
        return r;
    }
    inline MxVBlock<T, N> operator/(T s) const
    {
        MxVBlock<T, N> r;
        mxv_invscale(r, elt, s, N);
        return r;
    }
    inline MxVBlock<T, N> operator-() const
    {
        MxVBlock<T, N> r;
        mxv_neg(r, elt, N);
        return r;
    }

    inline T operator*(const MxVBlock<T, N>& v) const { return mxv_dot(elt, v, N); }
    // Comparison operators
    inline bool operator==(const MxVBlock<T, N>& v) const { return mxv_equal(elt, v, N); }
    inline bool operator!=(const MxVBlock<T, N>& v) const { return !mxv_equal(elt, v, N); }
};

////////////////////////////////////////////////////////////////////////
//
// MxVector -- arbitrarily sized vectors
//
// With MxVector vectors, we're willing to pay a little per-vector space
// overhead to achieve greater flexibility without having to instantiate
// a template class for every single dimension we care about.
//

#include "MxBlock.h"

class MxVector : public MxBlock<double>
{
public:
    MxVector(unsigned int n) : MxBlock<double>(n) { mxv_set(*this, 0.0, dim()); }
    MxVector(const MxVector& v) : MxBlock<double>(v.length()) { copy(v); }
    MxVector& operator=(const MxVector& v)
    {
        copy(v);
        return *this;
    }
    MxVector& operator=(double d)
    {
        mxv_set(*this, d, dim());
        return *this;
    }

    unsigned int dim() const { return length(); }
    MxVector& operator+=(const MxVector& v)
    {
        mxv_addinto(*this, v, dim());
        return *this;
    }
    MxVector& operator-=(const MxVector& v)
    {
        mxv_subfrom(*this, v, dim());
        return *this;
    }
    MxVector& operator*=(double d)
    {
        mxv_scale(*this, d, dim());
        return *this;
    }
    MxVector& operator/=(double d)
    {
        mxv_invscale(*this, d, dim());
        return *this;
    }

    double operator*(const MxVector& v) const { return mxv_dot(*this, v, dim()); }
};

// Convenient wrappers for mixvops functionality
//
inline double norm(const MxVector& v) { return mxv_norm(v, v.dim()); }
inline double norm2(const MxVector& v) { return mxv_dot(v, v, v.dim()); }
inline double unitize(MxVector& v) { return mxv_unitize(v, v.dim()); }
// MXVECTOR_INCLUDED
#endif
