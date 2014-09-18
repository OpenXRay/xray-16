// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLMATHH
#define WMLMATHH

#include "WmlSystem.h"

namespace Wml
{

template <class Real>
class Math
{
public:
    // Wrappers to hide implementations of functions.  The ACos and ASin
    // functions clamp the input argument to [-1,1] to avoid NaN issues
    // when the input is slightly larger than 1 or slightly smaller than -1.
    // Other functions have the potential for using a fast and approximate
    // algorithm rather than calling the standard math library functions.
    static Real ACos (Real fValue);
    static Real ASin (Real fValue);
    static Real ATan (Real fValue);
    static Real ATan2 (Real fY, Real fX);
    static Real Ceil (Real fValue);
    static Real Cos (Real fValue);
    static Real Exp (Real fValue);
    static Real FAbs (Real fValue);
    static Real Floor (Real fValue);
    static Real FMod (Real fX, Real fY);
    static Real InvSqrt (Real fValue);
    static Real Log (Real fValue);
    static Real Pow (Real fBase, Real fExponent);
    static Real Sin (Real fValue);
    static Real Sqr (Real fValue);
    static Real Sqrt (Real fValue);
    static Real Tan (Real fValue);

    // Return -1 if the input is negative, 0 if the input is zero, and +1
    // if the input is positive.
    static int Sign (int iValue);
    static Real Sign (Real fValue);

    // Generate a random number in [0,1).  The random number generator may
    // be seeded by a first call to UnitRandom with a positive seed.
    static Real UnitRandom (unsigned int uiSeed = 0);

    // Generate a random number in [-1,1).  The random number generator may
    // be seeded by a first call to SymmetricRandom with a positive seed.
    static Real SymmetricRandom (unsigned int uiSeed = 0);

    // Generate a random number in [min,max).  The random number generator may
    // be seeded by a first call to IntervalRandom with a positive seed.
    static Real IntervalRandom (Real fMin, Real fMax,
        unsigned int uiSeed = 0);

    // Fast evaluation of sin(angle) by polynomial approximations.  The angle
    // must be in [0,pi/2].  The maximum absolute error is about 1.7e-04 for
    // FastSin0 and about 2.3e-09 for FastSin1.  The speed up is about 2 for
    // FastSin0 and about 1.5 for FastSin1.
    static Real FastSin0 (Real fAngle);
    static Real FastSin1 (Real fAngle);

    // Fast evaluation of cos(angle) by polynomial approximations.  The angle
    // must be in [0,pi/2].  The maximum absolute error is about 1.2e-03 for
    // FastCos0 and about 2.3e-09 for FastCos1.  The speed up is about 2 for
    // FastCos0 and about 1.5 for FastCos1.
    static Real FastCos0 (Real fAngle);
    static Real FastCos1 (Real fAngle);

    // Fast evaluation of tan(angle) by polynomial approximations.  The angle
    // must be in [0,pi/4].  The maximum absolute error is about 8.1e-04 for
    // FastTan0 and about 1.9e-08 for FastTan1.  The speed up is about 2.5 for
    // FastTan0 and about 1.75 for FastTan1.
    static Real FastTan0 (Real fAngle);
    static Real FastTan1 (Real fAngle);

    // Fast evaluation of asin(value) by a sqrt times a polynomial.  The value
    // must be in [0,1].  The maximum absolute error is about 6.8e-05 and the
    // speed up is about 2.5.
    static Real FastInvSin (Real fValue);

    // Fast evaluation of acos(value) by a sqrt times a polynomial.  The value
    // must be in [0,1].  The maximum absolute error is about 6.8e-05 and the
    // speed up is about 2.5.
    static Real FastInvCos (Real fValue);

    // Fast evaluation of atan(value) by polynomial approximations.  The value
    // must be in [-1,1].  The maximum absolute error is about 1.2e-05 for
    // FastInvTan0 and about 1.43-08 for FastInvTan1.  The speed up is about
    // 2.2 for FastInvTan0 and about 1.5 for FastInvTan1.
    static Real FastInvTan0 (Real fValue);
    static Real FastInvTan1 (Real fValue);

    // A fast approximation to 1/sqrt.
    static Real FastInvSqrt (Real fValue);

    // gamma and related functions
    static Real LogGamma (Real fX);
    static Real Gamma (Real fX);
    static Real IncompleteGamma (Real fA, Real fX);

    // error functions
    static Real Erf (Real fX);   // polynomial approximation
    static Real Erfc (Real fX);  // erfc(x) = 1-erf(x)

    // modified Bessel functions of order 0 and 1
    static Real ModBessel0 (Real fX);
    static Real ModBessel1 (Real fX);

    // common constants
    WML_ITEM static const Real EPSILON;
    WML_ITEM static const Real ZERO_TOLERANCE;
    WML_ITEM static const Real MAX_REAL;
	WML_ITEM static const Real _PI;
    WML_ITEM static const Real TWO_PI;
    WML_ITEM static const Real HALF_PI;
    WML_ITEM static const Real INV_PI;
    WML_ITEM static const Real INV_TWO_PI;
    WML_ITEM static const Real DEG_TO_RAD;
    WML_ITEM static const Real RAD_TO_DEG;

protected:
    // series form (used when fX < 1+fA)
    static Real IncompleteGammaS (Real fA, Real fX);

    // continued fraction form (used when fX >= 1+fA)
    static Real IncompleteGammaCF (Real fA, Real fX);
};

#include "WmlMath.inl"

typedef Math<float> Mathf;
typedef Math<double> Mathd;

}

#endif
