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
#include <math.h>
//----------------------------------------------------------------------------
#define M_PI 3.14159265358979323846 /* pi */

inline Real Math::ACos(Real fValue)
{
    if (-1.0f < fValue)
    {
        if (fValue < 1.0f)
            return (Real)acos(fValue);
        else
            return 0.0f;
    }
    else
    {
        return M_PI;
    }
}
//----------------------------------------------------------------------------
inline Real Math::ASin(Real fValue)
{
    if (-1.0f < fValue)
    {
        if (fValue < 1.0f)
            return (Real)asin(fValue);
        else
            return -HALF_PI;
    }
    else
    {
        return HALF_PI;
    }
}
//----------------------------------------------------------------------------
inline Real Math::ATan(Real fValue)
{
    return (Real)atan(fValue);
}
//----------------------------------------------------------------------------
inline Real Math::ATan2(Real fY, Real fX)
{
    return (Real)atan2(fY, fX);
}
//----------------------------------------------------------------------------
inline Real Math::Ceil(Real fValue)
{
    return (Real)ceil(fValue);
}
//----------------------------------------------------------------------------
inline Real Math::Cos(Real fValue)
{
    return (Real)cos(fValue);
}
//----------------------------------------------------------------------------
inline Real Math::Exp(Real fValue)
{
    return (Real)exp(fValue);
}
//----------------------------------------------------------------------------
inline Real Math::FAbs(Real fValue)
{
    return (Real)fabs(fValue);
}
//----------------------------------------------------------------------------
inline Real Math::Floor(Real fValue)
{
    return (Real)floor(fValue);
}
//----------------------------------------------------------------------------
inline Real Math::InvSqrt(Real fValue)
{
    return (Real)(1.0 / sqrt(fValue));
}
//----------------------------------------------------------------------------
inline Real Math::Log(Real fValue)
{
    return (Real)log(fValue);
}
//----------------------------------------------------------------------------
inline Real Math::Pow(Real kBase, Real kExponent)
{
    return (Real)pow(kBase, kExponent);
}
//----------------------------------------------------------------------------
inline int Math::Sign(int iValue)
{
    if (iValue > 0)
        return 1;

    if (iValue < 0)
        return -1;

    return 0;
}
//----------------------------------------------------------------------------
inline Real Math::Sign(Real fValue)
{
    if (fValue > 0.0f)
        return 1.0f;

    if (fValue < 0.0f)
        return -1.0f;

    return 0.0f;
}
//----------------------------------------------------------------------------
inline Real Math::Sin(Real fValue)
{
    return (Real)sin(fValue);
}
//----------------------------------------------------------------------------
inline Real Math::Sqr(Real fValue)
{
    return fValue * fValue;
}
//----------------------------------------------------------------------------
inline Real Math::Sqrt(Real fValue)
{
    return (Real)sqrt(fValue);
}
//----------------------------------------------------------------------------
inline Real Math::Tan(Real fValue)
{
    return (Real)tan(fValue);
}
//----------------------------------------------------------------------------
