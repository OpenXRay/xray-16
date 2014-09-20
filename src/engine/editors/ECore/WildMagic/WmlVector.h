// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLVECTOR_H
#define WMLVECTOR_H

#include "WmlMath.h"

namespace Wml
{

template <int N, class Real>
class Vector
{
public:
    // construction
    Vector ();
    Vector (const Real* afTuple);
    Vector (const Vector& rkV);

    // coordinate access
    operator const Real* () const;
    operator Real* ();
    Real operator[] (int i) const;
    Real& operator[] (int i);

    // assignment
    Vector& operator= (const Vector& rkV);

    // comparison
    bool operator== (const Vector& rkV) const;
    bool operator!= (const Vector& rkV) const;
    bool operator<  (const Vector& rkV) const;
    bool operator<= (const Vector& rkV) const;
    bool operator>  (const Vector& rkV) const;
    bool operator>= (const Vector& rkV) const;

    // arithmetic operations
    Vector operator+ (const Vector& rkV) const;
    Vector operator- (const Vector& rkV) const;
    Vector operator* (Real fScalar) const;
    Vector operator/ (Real fScalar) const;
    Vector operator- () const;

    // arithmetic updates
    Vector& operator+= (const Vector& rkV);
    Vector& operator-= (const Vector& rkV);
    Vector& operator*= (Real fScalar);
    Vector& operator/= (Real fScalar);

    // vector operations
    Real Length () const;
    Real SquaredLength () const;
    Real Dot (const Vector& rkV) const;
    Real Normalize ();

protected:
    // support for comparisons
    int CompareArrays (const Vector& rkV) const;

    Real m_afTuple[N];
};

template <int N, class Real>
Vector<N,Real> operator* (Real fScalar, const Vector<N,Real>& rkV);

#include "WmlVector.inl"

}

#endif
