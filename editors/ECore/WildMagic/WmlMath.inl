// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ACos (Real fValue)
{
    if ( -(Real)1.0 < fValue )
    {
        if ( fValue < (Real)1.0 )
            return (Real)acos((double)fValue);
        else
            return (Real)0.0;
    }
    else
    {
        return PI;
    }
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ASin (Real fValue)
{
    if ( -(Real)1.0 < fValue )
    {
        if ( fValue < (Real)1.0 )
            return (Real)asin((double)fValue);
        else
            return HALF_PI;
    }
    else
    {
        return -HALF_PI;
    }
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ATan (Real fValue)
{
    return (Real)atan((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ATan2 (Real fY, Real fX)
{
    return (Real)atan2((double)fY,(double)fX);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Ceil (Real fValue)
{
    return (Real)ceil((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Cos (Real fValue)
{
    return (Real)cos((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Exp (Real fValue)
{
    return (Real)exp((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FAbs (Real fValue)
{
    return (Real)fabs((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Floor (Real fValue)
{
    return (Real)floor((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FMod (Real fX, Real fY)
{
    return (Real)fmod((double)fX,(double)fY);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::InvSqrt (Real fValue)
{
    return (Real)(1.0/sqrt((double)fValue));
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Log (Real fValue)
{
    return (Real)log((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Pow (Real fBase, Real fExponent)
{
    return (Real)pow((double)fBase,(double)fExponent);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sin (Real fValue)
{
    return (Real)sin((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sqr (Real fValue)
{
    return fValue*fValue;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sqrt (Real fValue)
{
    return (Real)sqrt((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Tan (Real fValue)
{
    return (Real)tan((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
int Math<Real>::Sign (int iValue)
{
    if ( iValue > 0 )
        return 1;

    if ( iValue < 0 )
        return -1;

    return 0;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sign (Real fValue)
{
    if ( fValue > (Real)0.0 )
        return (Real)1.0;

    if ( fValue < (Real)0.0 )
        return -(Real)1.0;

    return (Real)0.0;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::UnitRandom (unsigned int uiSeed )
{
    if ( uiSeed > 0 )
        srand(uiSeed);

    double dRatio = ((double)rand())/((double)(RAND_MAX));
    return (Real)dRatio;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::SymmetricRandom (unsigned int uiSeed)
{
    if ( uiSeed > 0.0 )
        srand(uiSeed);

    double dRatio = ((double)rand())/((double)(RAND_MAX));
    return (Real)(2.0*dRatio - 1.0);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::IntervalRandom (Real fMin, Real fMax, unsigned int uiSeed)
{
    if ( uiSeed > 0 )
        srand(uiSeed);

    double dRatio = ((double)rand())/((double)(RAND_MAX));
    return fMin+(fMax-fMin)*((Real)dRatio);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastSin0 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)7.61e-03;
    fResult *= fASqr;
    fResult -= (Real)1.6605e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastSin1 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = -(Real)2.39e-08;
    fResult *= fASqr;
    fResult += (Real)2.7526e-06;
    fResult *= fASqr;
    fResult -= (Real)1.98409e-04;
    fResult *= fASqr;
    fResult += (Real)8.3333315e-03;
    fResult *= fASqr;
    fResult -= (Real)1.666666664e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastCos0 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)3.705e-02;
    fResult *= fASqr;
    fResult -= (Real)4.967e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastCos1 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = -(Real)2.605e-07;
    fResult *= fASqr;
    fResult += (Real)2.47609e-05;
    fResult *= fASqr;
    fResult -= (Real)1.3888397e-03;
    fResult *= fASqr;
    fResult += (Real)4.16666418e-02;
    fResult *= fASqr;
    fResult -= (Real)4.999999963e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastTan0 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)2.033e-01;
    fResult *= fASqr;
    fResult += (Real)3.1755e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastTan1 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)9.5168091e-03;
    fResult *= fASqr;
    fResult += (Real)2.900525e-03;
    fResult *= fASqr;
    fResult += (Real)2.45650893e-02;
    fResult *= fASqr;
    fResult += (Real)5.33740603e-02;
    fResult *= fASqr;
    fResult += (Real)1.333923995e-01;
    fResult *= fASqr;
    fResult += (Real)3.333314036e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvSin (Real fValue)
{
    Real fRoot = Math<Real>::Sqrt(((Real)1.0)-fValue);
    Real fResult = -(Real)0.0187293;
    fResult *= fValue;
    fResult += (Real)0.0742610;
    fResult *= fValue;
    fResult -= (Real)0.2121144;
    fResult *= fValue;
    fResult += (Real)1.5707288;
    fResult = HALF_PI - fRoot*fResult;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvCos (Real fValue)
{
    Real fRoot = Math<Real>::Sqrt(((Real)1.0)-fValue);
    Real fResult = -(Real)0.0187293;
    fResult *= fValue;
    fResult += (Real)0.0742610;
    fResult *= fValue;
    fResult -= (Real)0.2121144;
    fResult *= fValue;
    fResult += (Real)1.5707288;
    fResult *= fRoot;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvTan0 (Real fValue)
{
    Real fVSqr = fValue*fValue;
    Real fResult = (Real)0.0208351;
    fResult *= fVSqr;
    fResult -= (Real)0.085133;
    fResult *= fVSqr;
    fResult += (Real)0.180141;
    fResult *= fVSqr;
    fResult -= (Real)0.3302995;
    fResult *= fVSqr;
    fResult += (Real)0.999866;
    fResult *= fValue;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvTan1 (Real fValue)
{
    Real fVSqr = fValue*fValue;
    Real fResult = (Real)0.0028662257;
    fResult *= fVSqr;
    fResult -= (Real)0.0161657367;
    fResult *= fVSqr;
    fResult += (Real)0.0429096138;
    fResult *= fVSqr;
    fResult -= (Real)0.0752896400;
    fResult *= fVSqr;
    fResult += (Real)0.1065626393;
    fResult *= fVSqr;
    fResult -= (Real)0.1420889944;
    fResult *= fVSqr;
    fResult += (Real)0.1999355085;
    fResult *= fVSqr;
    fResult -= (Real)0.3333314528;
    fResult *= fVSqr;
    fResult += (Real)1.0;
    fResult *= fValue;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvSqrt (Real fValue)
{
    // TO DO.  This routine was designed for 'float'.  Come up with an
    // equivalent one for 'double' and specialize the templates for 'float'
    // and 'double'.
    float fFValue = (float)fValue;
    float fHalf = 0.5f*fFValue;
    int i  = *(int*)&fFValue;
    i = 0x5f3759df - (i >> 1);
    fFValue = *(float*)&i;
    fFValue = fFValue*(1.5f - fHalf*fFValue*fFValue);
    return (Real)fFValue;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::LogGamma (Real fX)
{
    const Real afCoeff[6] =
    {
        +(Real)76.18009173,
        -(Real)86.50532033,
        +(Real)24.01409822,
        -(Real)1.231739516,
        +(Real)0.120858003e-02,
        -(Real)0.536382000e-05
    };

    fX -= (Real)1.0;
    Real fTmp = fX + (Real)5.5;
    fTmp -= (fX+((Real)0.5))*Math::Log(fTmp);
    Real fSeries = (Real)1.0;
    for (int j = 0; j <= 5; j++)
    {
        fX += (Real)1.0;
        fSeries += afCoeff[j]/fX;
    }
    return -fTmp + Math::Log(((Real)2.50662827465)*fSeries);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Gamma (Real fX)
{
    return Math::Exp(LogGamma(fX));
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::IncompleteGammaS (Real fA, Real fX)
{
    const int iMaxIterations = 100;
    const Real fTolerance = (Real)3e-07;

    if ( fX > (Real)0.0 )
    {
        Real fAp = fA;
        Real fSum = ((Real)1.0)/fA, fDel = fSum;
        for (int i = 1; i <= iMaxIterations; i++)
        {
            fAp += (Real)1.0;
            fDel *= fX/fAp;
            fSum += fDel;
            if ( Math::FAbs(fDel) < Math::FAbs(fSum)*fTolerance )
            {
                Real fArg = -fX+fA*Math::Log(fX)-LogGamma(fA);
                return fSum*Math::Exp(fArg);
            }
        }
    }

    if ( fX == (Real)0.0 )
        return (Real)0.0;

    return Math::MAX_REAL; // LogGamma not defined for x < 0
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::IncompleteGammaCF (Real fA, Real fX)
{
    const int iMaxIterations = 100;
    const Real fTolerance = (Real)3e-07;

    Real fA0 = (Real)1.0, fA1 = fX;
    Real fB0 = 0, fB1 = (Real)1.0;
    Real fGold = (Real)0.0, fFac = (Real)1.0;

    for (int i = 1; i <= iMaxIterations; i++)
    {
        Real fI = (Real) i;
        Real fImA = fI - fA;
        fA0 = (fA1 + fA0*fImA)*fFac;
        fB0 = (fB1 + fB0*fImA)*fFac;
        Real fItF = fI*fFac;
        fA1 = fX*fA0 + fItF*fA1;
        fB1 = fX*fB0 + fItF*fB1;
        if ( fA1 != (Real)0.0 )
        {
            fFac = ((Real)1.0)/fA1;
            Real fG = fB1*fFac;
            if ( Math::FAbs((fG-fGold)/fG) < fTolerance)
            {
                Real fArg = -fX + fA*Math::Log(fX) - LogGamma(fA);
                return fG*Math::Exp(fArg);
            }
            fGold = fG;
        }
    }

    return Math::MAX_REAL;  // numerical error if you get here
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::IncompleteGamma (Real fA, Real fX)
{
    if ( fX < (Real)1.0 + fA )
        return IncompleteGammaS(fA,fX);
    else
        return (Real)1.0-IncompleteGammaCF(fA,fX);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Erf (Real fX)
{
    return (Real)1.0-Erfc(fX);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Erfc (Real fX)
{
    const Real afCoeff[10] =
    {
        -(Real)1.26551223,
        +(Real)1.00002368,
        +(Real)0.37409196,
        +(Real)0.09678418,
        -(Real)0.18628806,
        +(Real)0.27886807,
        -(Real)1.13520398,
        +(Real)1.48851587,
        -(Real)0.82215223,
        +(Real)0.17087277
    };

    Real fZ = Math::FAbs(fX);
    Real fT = ((Real)1.0)/((Real)1.0+((Real)0.5)*fZ);
    Real fSum = afCoeff[9];

    for (int i = 9; i >= 0; i--)
        fSum = fT*fSum + afCoeff[i];

    Real fResult = fT*Math::Exp(-fZ*fZ + fSum);

    return ( fX >= (Real)0.0 ? fResult : (Real)2.0 - fResult );
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ModBessel0 (Real fX)
{
    if ( fX < (Real)0.0 )  // function is even
        fX = -fX;

    Real fT, fResult;
    int i;

    if ( fX <= (Real)3.75 )
    {
        const Real afCoeff[7] =
        {
            (Real)1.0000000,
            (Real)3.5156229,
            (Real)3.0899424,
            (Real)1.2067492,
            (Real)0.2659732,
            (Real)0.0360768,
            (Real)0.0045813
        };

        fT = fX/(Real)3.75;
        Real fT2 = fT*fT;
        fResult = afCoeff[6];
        for (i = 5; i >= 0; i--)
        {
            fResult *= fT2;
            fResult += afCoeff[i];
        }
        // |error| < 1.6e-07
    }
    else
    {
        const Real afCoeff[9] =
        {
            +(Real)0.39894228,
            +(Real)0.01328592,
            +(Real)0.00225319,
            -(Real)0.00157565,
            +(Real)0.00916281,
            -(Real)0.02057706,
            +(Real)0.02635537,
            -(Real)0.01647633,
            +(Real)0.00392377
        };

        fT = fX/(Real)3.75;
        Real fInvT = ((Real)1.0)/fT;
        fResult = afCoeff[8];
        for (i = 7; i >= 0; i--)
        {
            fResult *= fInvT;
            fResult += afCoeff[i];
        }
        fResult *= Math::Exp(fX);
        fResult /= Math::Sqrt(fX);
        // |error| < 1.9e-07
    }

    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ModBessel1 (Real fX)
{
    int iSign;
    if ( fX > (Real)0.0 )
    {
        iSign = 1;
    }
    else if ( fX < (Real)0.0 )
    {
        fX = -fX;
        iSign = -1;
    }
    else
    {
        return (Real)0.0;
    }

    Real fT, fResult;
    int i;

    if ( fX <= (Real)3.75 )
    {
        const Real afCoeff[7] =
        {
            (Real)0.50000000,
            (Real)0.87890549,
            (Real)0.51498869,
            (Real)0.15084934,
            (Real)0.02658733,
            (Real)0.00301532,
            (Real)0.00032411
        };

        fT = fX/(Real)3.75;
        Real fT2 = fT*fT;
        fResult = afCoeff[6];
        for (i = 5; i >= 0; i--)
        {
            fResult *= fT2;
            fResult += afCoeff[i];
        }
        fResult *= fX;
        // |error| < 8e-09
    }
    else
    {
        const Real afCoeff[9] =
        {
            +(Real)0.39894228,
            -(Real)0.03988024,
            -(Real)0.00362018,
            +(Real)0.00163801,
            -(Real)0.01031555,
            +(Real)0.02282967,
            -(Real)0.02895312,
            +(Real)0.01787654,
            -(Real)0.00420059
        };

        fT = fX/(Real)3.75;
        Real fInvT = ((Real)1.0)/fT;
        fResult = afCoeff[8];
        for (i = 7; i >= 0; i--)
        {
            fResult *= fInvT;
            fResult += afCoeff[i];
        }
        fResult *= Math::Exp(fX);
        fResult /= Math::Sqrt(fX);
        // |error| < 2.2e-07
    }

    fResult *= iSign;
    return fResult;
}
//----------------------------------------------------------------------------
