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

#include "MgcVector3.h"
using namespace Mgc;

const Vector3 Vector3::ZERO(0.0f, 0.0f, 0.0f);
const Vector3 Vector3::UNIT_X(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::UNIT_Y(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::UNIT_Z(0.0f, 0.0f, 1.0f);
Real Vector3::FUZZ = 0.0f;

//----------------------------------------------------------------------------
Vector3::Vector3(Real fX, Real fY, Real fZ)
{
    x = fX;
    y = fY;
    z = fZ;
}
//----------------------------------------------------------------------------
Vector3::Vector3(Real afCoordinate[3])
{
    x = afCoordinate[0];
    y = afCoordinate[1];
    z = afCoordinate[2];
}
//----------------------------------------------------------------------------
Vector3::Vector3(const Vector3 &rkVector)
{
    x = rkVector.x;
    y = rkVector.y;
    z = rkVector.z;
}
//----------------------------------------------------------------------------
Vector3 &Vector3::operator=(const Vector3 &rkVector)
{
    x = rkVector.x;
    y = rkVector.y;
    z = rkVector.z;
    return *this;
}
//----------------------------------------------------------------------------
bool Vector3::operator==(const Vector3 &rkVector) const
{
    if (FUZZ == 0.0f)
    {
        return x == rkVector.x && y == rkVector.y && z == rkVector.z;
    }
    else
    {
        return Math::FAbs(x - rkVector.x) <= FUZZ && Math::FAbs(y - rkVector.y) <= FUZZ && Math::FAbs(z - rkVector.z) <= FUZZ;
    }
}
//----------------------------------------------------------------------------
bool Vector3::operator!=(const Vector3 &rkVector) const
{
    if (FUZZ == 0.0f)
    {
        return x != rkVector.x || y != rkVector.y || z != rkVector.z;
    }
    else
    {
        return Math::FAbs(x - rkVector.x) > FUZZ || Math::FAbs(y - rkVector.y) > FUZZ || Math::FAbs(z - rkVector.z) > FUZZ;
    }
}
//----------------------------------------------------------------------------
bool Vector3::operator<(const Vector3 &rkVector) const
{
    Real fXTmp = rkVector.x, fYTmp = rkVector.y, fZTmp = rkVector.z;
    if (FUZZ > 0.0f)
    {
        if (Math::FAbs(x - fXTmp) <= FUZZ)
            fXTmp = x;
        if (Math::FAbs(y - fYTmp) <= FUZZ)
            fYTmp = y;
        if (Math::FAbs(z - fZTmp) <= FUZZ)
            fZTmp = z;
    }

    // compare z values
    unsigned int uiTest0 = *(unsigned int *)&z;
    unsigned int uiTest1 = *(unsigned int *)&fZTmp;
    if (uiTest0 < uiTest1)
        return true;
    if (uiTest0 > uiTest1)
        return false;

    // compare y values
    uiTest0 = *(unsigned int *)&y;
    uiTest1 = *(unsigned int *)&fYTmp;
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
bool Vector3::operator<=(const Vector3 &rkVector) const
{
    Real fXTmp = rkVector.x, fYTmp = rkVector.y, fZTmp = rkVector.z;
    if (FUZZ > 0.0f)
    {
        if (Math::FAbs(x - fXTmp) <= FUZZ)
            fXTmp = x;
        if (Math::FAbs(y - fYTmp) <= FUZZ)
            fYTmp = y;
        if (Math::FAbs(z - fZTmp) <= FUZZ)
            fZTmp = z;
    }

    // compare z values
    unsigned int uiTest0 = *(unsigned int *)&z;
    unsigned int uiTest1 = *(unsigned int *)&fZTmp;
    if (uiTest0 < uiTest1)
        return true;
    if (uiTest0 > uiTest1)
        return false;

    // compare y values
    uiTest0 = *(unsigned int *)&y;
    uiTest1 = *(unsigned int *)&fYTmp;
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
bool Vector3::operator>(const Vector3 &rkVector) const
{
    Real fXTmp = rkVector.x, fYTmp = rkVector.y, fZTmp = rkVector.z;
    if (FUZZ > 0.0f)
    {
        if (Math::FAbs(x - fXTmp) <= FUZZ)
            fXTmp = x;
        if (Math::FAbs(y - fYTmp) <= FUZZ)
            fYTmp = y;
        if (Math::FAbs(z - fZTmp) <= FUZZ)
            fZTmp = z;
    }

    // compare z values
    unsigned int uiTest0 = *(unsigned int *)&z;
    unsigned int uiTest1 = *(unsigned int *)&fZTmp;
    if (uiTest0 > uiTest1)
        return true;
    if (uiTest0 < uiTest1)
        return false;

    // compare y values
    uiTest0 = *(unsigned int *)&y;
    uiTest1 = *(unsigned int *)&fYTmp;
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
bool Vector3::operator>=(const Vector3 &rkVector) const
{
    Real fXTmp = rkVector.x, fYTmp = rkVector.y, fZTmp = rkVector.z;
    if (FUZZ > 0.0f)
    {
        if (Math::FAbs(x - fXTmp) <= FUZZ)
            fXTmp = x;
        if (Math::FAbs(y - fYTmp) <= FUZZ)
            fYTmp = y;
        if (Math::FAbs(z - fZTmp) <= FUZZ)
            fZTmp = z;
    }

    // compare z values
    unsigned int uiTest0 = *(unsigned int *)&z;
    unsigned int uiTest1 = *(unsigned int *)&fZTmp;
    if (uiTest0 > uiTest1)
        return true;
    if (uiTest0 < uiTest1)
        return false;

    // compare y values
    uiTest0 = *(unsigned int *)&y;
    uiTest1 = *(unsigned int *)&fYTmp;
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
Vector3 Vector3::operator+(const Vector3 &rkVector) const
{
    return Vector3(x + rkVector.x, y + rkVector.y, z + rkVector.z);
}
//----------------------------------------------------------------------------
Vector3 Vector3::operator-(const Vector3 &rkVector) const
{
    return Vector3(x - rkVector.x, y - rkVector.y, z - rkVector.z);
}
//----------------------------------------------------------------------------
Vector3 Vector3::operator*(Real fScalar) const
{
    return Vector3(fScalar * x, fScalar * y, fScalar * z);
}
//----------------------------------------------------------------------------
Vector3 Vector3::operator/(Real fScalar) const
{
    Vector3 kQuot;

    if (fScalar != 0.0f)
    {
        Real fInvScalar = 1.0f / fScalar;
        kQuot.x = fInvScalar * x;
        kQuot.y = fInvScalar * y;
        kQuot.z = fInvScalar * z;
        return kQuot;
    }
    else
    {
        return Vector3(Math::MAX_REAL, Math::MAX_REAL, Math::MAX_REAL);
    }
}
//----------------------------------------------------------------------------
Vector3 Vector3::operator-() const
{
    return Vector3(-x, -y, -z);
}
//----------------------------------------------------------------------------
Vector3 Mgc::operator*(Real fScalar, const Vector3 &rkVector)
{
    return Vector3(fScalar * rkVector.x, fScalar * rkVector.y,
                   fScalar * rkVector.z);
}
//----------------------------------------------------------------------------
Vector3 &Vector3::operator+=(const Vector3 &rkVector)
{
    x += rkVector.x;
    y += rkVector.y;
    z += rkVector.z;
    return *this;
}
//----------------------------------------------------------------------------
Vector3 &Vector3::operator-=(const Vector3 &rkVector)
{
    x -= rkVector.x;
    y -= rkVector.y;
    z -= rkVector.z;
    return *this;
}
//----------------------------------------------------------------------------
Vector3 &Vector3::operator*=(Real fScalar)
{
    x *= fScalar;
    y *= fScalar;
    z *= fScalar;
    return *this;
}
//----------------------------------------------------------------------------
Vector3 &Vector3::operator/=(Real fScalar)
{
    if (fScalar != 0.0f)
    {
        Real fInvScalar = 1.0f / fScalar;
        x *= fInvScalar;
        y *= fInvScalar;
        z *= fInvScalar;
    }
    else
    {
        x = Math::MAX_REAL;
        y = Math::MAX_REAL;
        z = Math::MAX_REAL;
    }

    return *this;
}
//----------------------------------------------------------------------------
Real Vector3::SquaredLength() const
{
    return x * x + y * y + z * z;
}
//----------------------------------------------------------------------------
Real Vector3::Length() const
{
    return Math::Sqrt(x * x + y * y + z * z);
}
//----------------------------------------------------------------------------
Real Vector3::Dot(const Vector3 &rkVector) const
{
    return x * rkVector.x + y * rkVector.y + z * rkVector.z;
}
//----------------------------------------------------------------------------
Vector3 Vector3::Cross(const Vector3 &rkVector) const
{
    return Vector3(y * rkVector.z - z * rkVector.y, z * rkVector.x - x * rkVector.z,
                   x * rkVector.y - y * rkVector.x);
}
//----------------------------------------------------------------------------
Vector3 Vector3::UnitCross(const Vector3 &rkVector) const
{
    Vector3 kCross(y * rkVector.z - z * rkVector.y, z * rkVector.x - x * rkVector.z,
                   x * rkVector.y - y * rkVector.x);
    kCross.Unitize();
    return kCross;
}
//----------------------------------------------------------------------------
Real Vector3::Unitize(Real fTolerance)
{
    Real fLength = Length();

    if (fLength > fTolerance)
    {
        Real fInvLength = 1.0f / fLength;
        x *= fInvLength;
        y *= fInvLength;
        z *= fInvLength;
    }
    else
    {
        fLength = 0.0f;
    }

    return fLength;
}
//----------------------------------------------------------------------------
void Vector3::Orthonormalize(Vector3 akVector[/*3*/])
{
    // If the input vectors are v0, v1, and v2, then the Gram-Schmidt
    // orthonormalization produces vectors u0, u1, and u2 as follows,
    //
    //   u0 = v0/|v0|
    //   u1 = (v1-(u0*v1)u0)/|v1-(u0*v1)u0|
    //   u2 = (v2-(u0*v2)u0-(u1*v2)u1)/|v2-(u0*v2)u0-(u1*v2)u1|
    //
    // where |A| indicates length of vector A and A*B indicates dot
    // product of vectors A and B.

    // compute u0
    akVector[0].Unitize();

    // compute u1
    Real fDot0 = akVector[0].Dot(akVector[1]);
    akVector[1] -= fDot0 * akVector[0];
    akVector[1].Unitize();

    // compute u2
    Real fDot1 = akVector[1].Dot(akVector[2]);
    fDot0 = akVector[0].Dot(akVector[2]);
    akVector[2] -= fDot0 * akVector[0] + fDot1 * akVector[1];
    akVector[2].Unitize();
}
//----------------------------------------------------------------------------
void Vector3::GenerateOrthonormalBasis(Vector3 &rkU, Vector3 &rkV,
                                       Vector3 &rkW, bool bUnitLengthW)
{
    if (!bUnitLengthW)
        rkW.Unitize();

    Real fInvLength;

    if (Math::FAbs(rkW.x) >= Math::FAbs(rkW.y))
    {
        // W.x or W.z is the largest magnitude component, swap them
        fInvLength = Math::InvSqrt(rkW.x * rkW.x + rkW.z * rkW.z);
        rkU.x = -rkW.z * fInvLength;
        rkU.y = 0.0f;
        rkU.z = +rkW.x * fInvLength;
    }
    else
    {
        // W.y or W.z is the largest magnitude component, swap them
        fInvLength = Math::InvSqrt(rkW.y * rkW.y + rkW.z * rkW.z);
        rkU.x = 0.0f;
        rkU.y = +rkW.z * fInvLength;
        rkU.z = -rkW.y * fInvLength;
    }

    rkV = rkW.Cross(rkU);
}
//----------------------------------------------------------------------------
