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
//#include "stdafx.h"
#pragma hdrstop

#include "MgcMath.h"
using namespace Mgc;

#ifdef MGC_USE_DOUBLE
const Real Mgc::Math::MAX_REAL = DBL_MAX;
#else
const Real Mgc::Math::MAX_REAL = FLT_MAX;
#endif

const Real Mgc::Math::_PI = 4.0f * Mgc::Math::ATan(1.0f);
const Real Mgc::Math::TWO_PI = 2.0f * _PI;
const Real Mgc::Math::HALF_PI = 0.5f * _PI;
const Real Mgc::Math::INV_TWO_PI = 1.0f / TWO_PI;
const Real Mgc::Math::DEG_TO_RAD = Mgc::Math::_PI / 180.0f;
const Real Mgc::Math::RAD_TO_DEG = 1.0f / Mgc::Math::DEG_TO_RAD;

//----------------------------------------------------------------------------
Real Mgc::Math::UnitRandom(Real fSeed)
{
    if (fSeed > 0.0f)
        srand((unsigned int)fSeed);

    return Real(rand()) / Real(RAND_MAX);
}
//----------------------------------------------------------------------------
Real Mgc::Math::SymmetricRandom(Real fSeed)
{
    if (fSeed > 0.0f)
        srand((unsigned int)fSeed);

    return 2.0f * Real(rand()) / Real(RAND_MAX) - 1.0f;
}
//----------------------------------------------------------------------------
Real Mgc::Math::IntervalRandom(Real fMin, Real fMax, Real fSeed)
{
    if (fSeed > 0.0f)
        srand((unsigned int)fSeed);

    return fMin + (fMax - fMin) * Real(rand()) / Real(RAND_MAX);
}
//----------------------------------------------------------------------------
Real Mgc::Math::FastSin0(Real fAngle)
{
    Real fASqr = fAngle * fAngle;
    Real fResult = 7.61e-03f;
    fResult *= fASqr;
    fResult -= 1.6605e-01f;
    fResult *= fASqr;
    fResult += 1.0f;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
Real Mgc::Math::FastSin1(Real fAngle)
{
    Real fASqr = fAngle * fAngle;
    Real fResult = -2.39e-08f;
    fResult *= fASqr;
    fResult += 2.7526e-06f;
    fResult *= fASqr;
    fResult -= 1.98409e-04f;
    fResult *= fASqr;
    fResult += 8.3333315e-03f;
    fResult *= fASqr;
    fResult -= 1.666666664e-01f;
    fResult *= fASqr;
    fResult += 1.0f;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
Real Mgc::Math::FastCos0(Real fAngle)
{
    Real fASqr = fAngle * fAngle;
    Real fResult = 3.705e-02f;
    fResult *= fASqr;
    fResult -= 4.967e-01f;
    fResult *= fASqr;
    fResult += 1.0f;
    return fResult;
}
//----------------------------------------------------------------------------
Real Mgc::Math::FastCos1(Real fAngle)
{
    Real fASqr = fAngle * fAngle;
    Real fResult = -2.605e-07f;
    fResult *= fASqr;
    fResult += 2.47609e-05f;
    fResult *= fASqr;
    fResult -= 1.3888397e-03f;
    fResult *= fASqr;
    fResult += 4.16666418e-02f;
    fResult *= fASqr;
    fResult -= 4.999999963e-01f;
    fResult *= fASqr;
    fResult += 1.0f;
    return fResult;
}
//----------------------------------------------------------------------------
Real Mgc::Math::FastTan0(Real fAngle)
{
    Real fASqr = fAngle * fAngle;
    Real fResult = 2.033e-01f;
    fResult *= fASqr;
    fResult += 3.1755e-01f;
    fResult *= fASqr;
    fResult += 1.0f;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
Real Mgc::Math::FastTan1(Real fAngle)
{
    Real fASqr = fAngle * fAngle;
    Real fResult = 9.5168091e-03f;
    fResult *= fASqr;
    fResult += 2.900525e-03f;
    fResult *= fASqr;
    fResult += 2.45650893e-02f;
    fResult *= fASqr;
    fResult += 5.33740603e-02f;
    fResult *= fASqr;
    fResult += 1.333923995e-01f;
    fResult *= fASqr;
    fResult += 3.333314036e-01f;
    fResult *= fASqr;
    fResult += 1.0f;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
Real Mgc::Math::FastInvSin(Real fValue)
{
    Real fRoot = Mgc::Math::Sqrt(1.0f - fValue);
    Real fResult = -0.0187293f;
    fResult *= fValue;
    fResult += 0.0742610f;
    fResult *= fValue;
    fResult -= 0.2121144f;
    fResult *= fValue;
    fResult += 1.5707288f;
    fResult = HALF_PI - fRoot * fResult;
    return fResult;
}
//----------------------------------------------------------------------------
Real Mgc::Math::FastInvCos(Real fValue)
{
    Real fRoot = Mgc::Math::Sqrt(1.0f - fValue);
    Real fResult = -0.0187293f;
    fResult *= fValue;
    fResult += 0.0742610f;
    fResult *= fValue;
    fResult -= 0.2121144f;
    fResult *= fValue;
    fResult += 1.5707288f;
    fResult *= fRoot;
    return fResult;
}
//----------------------------------------------------------------------------
Real Mgc::Math::FastInvTan0(Real fValue)
{
    Real fVSqr = fValue * fValue;
    Real fResult = 0.0208351f;
    fResult *= fVSqr;
    fResult -= 0.085133f;
    fResult *= fVSqr;
    fResult += 0.180141f;
    fResult *= fVSqr;
    fResult -= 0.3302995f;
    fResult *= fVSqr;
    fResult += 0.999866f;
    fResult *= fValue;
    return fResult;
}
//----------------------------------------------------------------------------
Real Mgc::Math::FastInvTan1(Real fValue)
{
    Real fVSqr = fValue * fValue;
    Real fResult = 0.0028662257f;
    fResult *= fVSqr;
    fResult -= 0.0161657367f;
    fResult *= fVSqr;
    fResult += 0.0429096138f;
    fResult *= fVSqr;
    fResult -= 0.0752896400f;
    fResult *= fVSqr;
    fResult += 0.1065626393f;
    fResult *= fVSqr;
    fResult -= 0.1420889944f;
    fResult *= fVSqr;
    fResult += 0.1999355085f;
    fResult *= fVSqr;
    fResult -= 0.3333314528f;
    fResult *= fVSqr;
    fResult += 1.0f;
    fResult *= fValue;
    return fResult;
}
//----------------------------------------------------------------------------
