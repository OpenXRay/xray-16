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
///#include "stdafx.h"
#pragma hdrstop

#include "MgcVector2.h"
using namespace Mgc;

const Vector2 Vector2::ZERO(0.0f, 0.0f);
const Vector2 Vector2::UNIT_X(1.0f, 0.0f);
const Vector2 Vector2::UNIT_Y(0.0f, 1.0f);
Real Vector2::FUZZ = 0.0f;

//----------------------------------------------------------------------------
Vector2::Vector2(Real fX, Real fY)
{
    x = fX;
    y = fY;
}
//----------------------------------------------------------------------------
Vector2::Vector2(Real afCoordinate[2])
{
    x = afCoordinate[0];
    y = afCoordinate[1];
}
//----------------------------------------------------------------------------
Vector2::Vector2(const Vector2 &rkVector)
{
    x = rkVector.x;
    y = rkVector.y;
}
//----------------------------------------------------------------------------
Vector2 &Vector2::operator=(const Vector2 &rkVector)
{
    x = rkVector.x;
    y = rkVector.y;
    return *this;
}
//----------------------------------------------------------------------------
bool Vector2::operator==(const Vector2 &rkVector) const
{
    if (FUZZ == 0.0f)
        return x == rkVector.x && y == rkVector.y;
    else
        return Math::FAbs(x - rkVector.x) <= FUZZ && Math::FAbs(y - rkVector.y) <= FUZZ;
}
//----------------------------------------------------------------------------
bool Vector2::operator!=(const Vector2 &rkVector) const
{
    if (FUZZ == 0.0f)
        return x != rkVector.x || y != rkVector.y;
    else
        return Math::FAbs(x - rkVector.x) > FUZZ || Math::FAbs(y - rkVector.y) > FUZZ;
}
//----------------------------------------------------------------------------
bool Vector2::operator<(const Vector2 &rkVector) const
{
    Real fXTmp = rkVector.x, fYTmp = rkVector.y;
    if (FUZZ > 0.0f)
    {
        if (Math::FAbs(x - fXTmp) <= FUZZ)
            fXTmp = x;
        if (Math::FAbs(y - fYTmp) <= FUZZ)
            fYTmp = y;
    }

    // compare y values
    unsigned int uiTest0 = *(unsigned int *)&y;
    unsigned int uiTest1 = *(unsigned int *)&fYTmp;
    if (uiTest0 < uiTest1)
        return true;
    if (uiTest0 > uiTest1)
        return false;

    // compare x values
    uiTest0 = *(unsigned int *)&x;
    uiTest1 = *(unsigned int *)&fXTmp;
    return uiTest0 < uiTest1;
}
//----------------------------------------------------------------------------
bool Vector2::operator<=(const Vector2 &rkVector) const
{
    Real fXTmp = rkVector.x, fYTmp = rkVector.y;
    if (FUZZ > 0.0f)
    {
        if (Math::FAbs(x - fXTmp) <= FUZZ)
            fXTmp = x;
        if (Math::FAbs(y - fYTmp) <= FUZZ)
            fYTmp = y;
    }

    // compare y values
    unsigned int uiTest0 = *(unsigned int *)&y;
    unsigned int uiTest1 = *(unsigned int *)&fYTmp;
    if (uiTest0 < uiTest1)
        return true;
    if (uiTest0 > uiTest1)
        return false;

    // compare x values
    uiTest0 = *(unsigned int *)&x;
    uiTest1 = *(unsigned int *)&fXTmp;
    return uiTest0 <= uiTest1;
}
//----------------------------------------------------------------------------
bool Vector2::operator>(const Vector2 &rkVector) const
{
    Real fXTmp = rkVector.x, fYTmp = rkVector.y;
    if (FUZZ > 0.0f)
    {
        if (Math::FAbs(x - fXTmp) <= FUZZ)
            fXTmp = x;
        if (Math::FAbs(y - fYTmp) <= FUZZ)
            fYTmp = y;
    }

    // compare y values
    unsigned int uiTest0 = *(unsigned int *)&y;
    unsigned int uiTest1 = *(unsigned int *)&fYTmp;
    if (uiTest0 > uiTest1)
        return true;
    if (uiTest0 < uiTest1)
        return false;

    // compare x values
    uiTest0 = *(unsigned int *)&x;
    uiTest1 = *(unsigned int *)&fXTmp;
    return uiTest0 > uiTest1;
}
//----------------------------------------------------------------------------
bool Vector2::operator>=(const Vector2 &rkVector) const
{
    Real fXTmp = rkVector.x, fYTmp = rkVector.y;
    if (FUZZ > 0.0f)
    {
        if (Math::FAbs(x - fXTmp) <= FUZZ)
            fXTmp = x;
        if (Math::FAbs(y - fYTmp) <= FUZZ)
            fYTmp = y;
    }

    // compare y values
    unsigned int uiTest0 = *(unsigned int *)&y;
    unsigned int uiTest1 = *(unsigned int *)&fYTmp;
    if (uiTest0 > uiTest1)
        return true;
    if (uiTest0 < uiTest1)
        return false;

    // compare x values
    uiTest0 = *(unsigned int *)&x;
    uiTest1 = *(unsigned int *)&fXTmp;
    return uiTest0 >= uiTest1;
}
//----------------------------------------------------------------------------
Vector2 Vector2::operator+(const Vector2 &rkVector) const
{
    return Vector2(x + rkVector.x, y + rkVector.y);
}
//----------------------------------------------------------------------------
Vector2 Vector2::operator-(const Vector2 &rkVector) const
{
    return Vector2(x - rkVector.x, y - rkVector.y);
}
//----------------------------------------------------------------------------
Vector2 Vector2::operator*(Real fScalar) const
{
    return Vector2(fScalar * x, fScalar * y);
}
//----------------------------------------------------------------------------
Vector2 Vector2::operator-() const
{
    return Vector2(-x, -y);
}
//----------------------------------------------------------------------------
Vector2 Mgc::operator*(Real fScalar, const Vector2 &rkVector)
{
    return Vector2(fScalar * rkVector.x, fScalar * rkVector.y);
}
//----------------------------------------------------------------------------
Vector2 Vector2::operator/(Real fScalar) const
{
    Vector2 kQuot;

    if (fScalar != 0.0f)
    {
        Real fInvScalar = 1.0f / fScalar;
        kQuot.x = fInvScalar * x;
        kQuot.y = fInvScalar * y;
        return kQuot;
    }
    else
    {
        return Vector2(Math::MAX_REAL, Math::MAX_REAL);
    }
}
//----------------------------------------------------------------------------
Vector2 &Vector2::operator+=(const Vector2 &rkVector)
{
    x += rkVector.x;
    y += rkVector.y;
    return *this;
}
//----------------------------------------------------------------------------
Vector2 &Vector2::operator-=(const Vector2 &rkVector)
{
    x -= rkVector.x;
    y -= rkVector.y;
    return *this;
}
//----------------------------------------------------------------------------
Vector2 &Vector2::operator*=(Real fScalar)
{
    x *= fScalar;
    y *= fScalar;
    return *this;
}
//----------------------------------------------------------------------------
Vector2 &Vector2::operator/=(Real fScalar)
{
    if (fScalar != 0.0f)
    {
        Real fInvScalar = 1.0f / fScalar;
        x *= fInvScalar;
        y *= fInvScalar;
    }
    else
    {
        x = Math::MAX_REAL;
        y = Math::MAX_REAL;
    }

    return *this;
}
//----------------------------------------------------------------------------
Real Vector2::Dot(const Vector2 &rkVector) const
{
    return x * rkVector.x + y * rkVector.y;
}
//----------------------------------------------------------------------------
Real Vector2::Length() const
{
    return Math::Sqrt(x * x + y * y);
}
//----------------------------------------------------------------------------
Vector2 Vector2::Cross() const
{
    return Vector2(y, -x);
}
//----------------------------------------------------------------------------
Vector2 Vector2::UnitCross() const
{
    Vector2 kCross(y, -x);
    kCross.Unitize();
    return kCross;
}
//----------------------------------------------------------------------------
Real Vector2::Unitize(Real fTolerance)
{
    Real fLength = Length();

    if (fLength > fTolerance)
    {
        Real fInvLength = 1.0f / fLength;
        x *= fInvLength;
        y *= fInvLength;
    }
    else
    {
        fLength = 0.0f;
    }

    return fLength;
}
//----------------------------------------------------------------------------
void Vector2::Orthonormalize(Vector2 akVector[/*2*/])
{
    // If the input vectors are v0 and v1, then the Gram-Schmidt
    // orthonormalization produces vectors u0 and u1 as follows,
    //
    //   u0 = v0/|v0|
    //   u1 = (v1-(u0*v1)u0)/|v1-(u0*v1)u0|
    //
    // where |A| indicates length of vector A and A*B indicates dot
    // product of vectors A and B.

    // compute u0
    akVector[0].Unitize();

    // compute u1
    Real fDot0 = akVector[0].Dot(akVector[1]);
    akVector[1] -= fDot0 * akVector[0];
    akVector[1].Unitize();
}
//----------------------------------------------------------------------------
