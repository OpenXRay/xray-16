// Magic Software, Inc.
// http://www.magic-software.com
// Copyright (c) 2000-2002.  All Rights Reserved
//
// Source code from Magic Software is supplied under the terms of a license
// agreement and may not be copied or disclosed except in accordance with the
// terms of that agreement.  The various license agreements may be found at
// the Magic Software web site.  This file is subject to the license
//
// FREE SOURCE CODE
// http://www.magic-software.com/License/free.pdf

#ifndef MGCMATH_H
#define MGCMATH_H

#include "MagicFMLibType.h"
#include "MgcRTLib.h"

namespace Mgc
{

#ifdef MGC_USE_DOUBLE
    typedef double Real;
#else
    typedef float Real;
#endif

    class MAGICFM Math
    {
    public:
        // Return -1 if the input is negative, 0 if the input is zero, and +1
        // if the input is positive.
        static int Sign(int iValue);
        static Real Sign(Real fValue);

        // Just computes fValue*fValue.
        static Real Sqr(Real fValue);

        // Generate a random number in [0,1).  The random number generator may
        // be seeded by a first call to UnitRandom with a positive seed.
        static Real UnitRandom(Real fSeed = 0.0f);

        // Generate a random number in [-1,1).  The random number generator may
        // be seeded by a first call to SymmetricRandom with a positive seed.
        static Real SymmetricRandom(Real fSeed = 0.0f);

        // Generate a random number in [min,max).  The random number generator may
        // be seeded by a first call to IntervalRandom with a positive seed.
        static Real IntervalRandom(Real fMin, Real fMax, Real fSeed = 0.0f);

        // Fast evaluation of sin(angle) by polynomial approximations.  The angle
        // must be in [0,pi/2].  The maximum absolute error is about 1.7e-04 for
        // FastSin0 and about 2.3e-09 for FastSin1.  The speed up is about 2 for
        // FastSin0 and about 1.5 for FastSin1.
        static Real FastSin0(Real fAngle);
        static Real FastSin1(Real fAngle);

        // Fast evaluation of cos(angle) by polynomial approximations.  The angle
        // must be in [0,pi/2].  The maximum absolute error is about 1.2e-03 for
        // FastCos0 and about 2.3e-09 for FastCos1.  The speed up is about 2 for
        // FastCos0 and about 1.5 for FastCos1.
        static Real FastCos0(Real fAngle);
        static Real FastCos1(Real fAngle);

        // Fast evaluation of tan(angle) by polynomial approximations.  The angle
        // must be in [0,pi/4].  The maximum absolute error is about 8.1e-04 for
        // FastTan0 and about 1.9e-08 for FastTan1.  The speed up is about 2.5 for
        // FastTan0 and about 1.75 for FastTan1.
        static Real FastTan0(Real fAngle);
        static Real FastTan1(Real fAngle);

        // Fast evaluation of asin(value) by a sqrt times a polynomial.  The value
        // must be in [0,1].  The maximum absolute error is about 6.8e-05 and the
        // speed up is about 2.5.
        static Real FastInvSin(Real fValue);

        // Fast evaluation of acos(value) by a sqrt times a polynomial.  The value
        // must be in [0,1].  The maximum absolute error is about 6.8e-05 and the
        // speed up is about 2.5.
        static Real FastInvCos(Real fValue);

        // Fast evaluation of atan(value) by polynomial approximations.  The value
        // must be in [-1,1].  The maximum absolute error is about 1.2e-05 for
        // FastInvTan0 and about 1.43-08 for FastInvTan1.  The speed up is about
        // 2.2 for FastInvTan0 and about 1.5 for FastInvTan1.
        static Real FastInvTan0(Real fValue);
        static Real FastInvTan1(Real fValue);

        // Wrappers for acos and asin, but the input value is clamped to [-1,1]
        // to avoid silent returns of NaN.
        static Real ACos(Real fValue);
        static Real ASin(Real fValue);

        // Wrapper for 1/sqrt to allow a fast implementation to replace it at a
        // later date.
        static Real InvSqrt(Real fValue);

        // Wrappers to hide 'double foo(double)' versus 'float foof(float)'.
        static Real ATan(Real fValue);
        static Real ATan2(Real fY, Real fX);
        static Real Ceil(Real fValue);
        static Real Cos(Real fValue);
        static Real Exp(Real fValue);
        static Real FAbs(Real fValue);
        static Real Floor(Real fValue);
        static Real Log(Real fValue);
        static Real Pow(Real kBase, Real kExponent);
        static Real Sin(Real fValue);
        static Real Sqrt(Real fValue);
        static Real Tan(Real fValue);

        // common constants
        static const Real MAX_REAL;
        static const Real _PI;
        static const Real TWO_PI;
        static const Real HALF_PI;
        static const Real INV_TWO_PI;
        static const Real DEG_TO_RAD;
        static const Real RAD_TO_DEG;
    };

#include "MgcMath.inl"

} // namespace Mgc

#endif
