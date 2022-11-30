#include "stdafx.h"

#include "xrCore/_cylinder.h"
#include "xrCommon/math_funcs_inline.h"

#ifdef DEBUG
#include "xrCore/xrDebug_macros.h"
#include "xrCore/log.h"
#endif

#include <limits>

int Fcylinder::intersect(const Fvector3& start, const Fvector3& dir, float afT[2], ecode code[2]) const
{
    constexpr float fEpsilon = 1e-12f;

    // set up quadratic Q(t) = a*t^2 + 2*b*t + c
    Fvector3 kU, kV, kW = m_direction;
    Fvector3::generate_orthonormal_basis(kW, kU, kV);
    Fvector3 kD;
    kD.set(kU.dotproduct(dir), kV.dotproduct(dir), kW.dotproduct(dir));
#ifdef DEBUG
    if (kD.square_magnitude() <= std::numeric_limits<float>::min())
    {
        Msg("dir :%f,%f,%f", dir.x, dir.y, dir.z);
        Msg("kU :%f,%f,%f", kU.x, kU.y, kU.z);
        Msg("kV :%f,%f,%f", kV.x, kV.y, kV.z);
        Msg("kW :%f,%f,%f", kW.x, kW.y, kW.z);
        VERIFY2(0, "KD is zero");
    }
#endif
    const float fDLength = kD.normalize_magn();
    const float fInvDLength = 1.0f / fDLength;
    Fvector3 kDiff;
    kDiff.sub(start, m_center);
    Fvector3 kP;
    kP.set(kU.dotproduct(kDiff), kV.dotproduct(kDiff), kW.dotproduct(kDiff));
    const float fHalfHeight = 0.5f * m_height;
    const float fRadiusSqr = m_radius * m_radius;

    float fInv, fA, fB, fC, fDiscr, fRoot, fT, fT0, fT1, fTmp0, fTmp1;

    if (_abs(kD.z) >= 1.0f - fEpsilon)
    {
        // line is parallel to cylinder axis
        if (kP.x * kP.x + kP.y * kP.y <= fRadiusSqr)
        {
            fTmp0 = fInvDLength / kD.z;
            afT[0] = (+fHalfHeight - kP.z) * fTmp0;
            afT[1] = (-fHalfHeight - kP.z) * fTmp0;
            code[0] = cyl_cap;
            code[1] = cyl_cap;
            return 2;
        }
        else
        {
            return 0;
        }
    }

    if (_abs(kD.z) <= fEpsilon)
    {
        // line is perpendicular to axis of cylinder
        if (_abs(kP.z) > fHalfHeight)
        {
            // line is outside the planar caps of cylinder
            return 0;
        }

        fA = kD.x * kD.x + kD.y * kD.y;
        fB = kP.x * kD.x + kP.y * kD.y;
        fC = kP.x * kP.x + kP.y * kP.y - fRadiusSqr;
        fDiscr = fB * fB - fA * fC;
        if (fDiscr < 0.0f)
        {
            // line does not intersect cylinder wall
            return 0;
        }
        else if (fDiscr > 0.0f)
        {
            fRoot = _sqrt(fDiscr);
            fTmp0 = fInvDLength / fA;
            afT[0] = (-fB - fRoot) * fTmp0;
            afT[1] = (-fB + fRoot) * fTmp0;
            code[0] = cyl_wall;
            code[1] = cyl_wall;
            return 2; // wall
        }
        else
        {
            afT[0] = -fB * fInvDLength / fA;
            code[0] = cyl_wall;
            return 1; // wall
        }
    }

    // test plane intersections first
    int iQuantity = 0;
    fInv = 1.0f / kD.z;
    fT0 = (+fHalfHeight - kP.z) * fInv;
    fTmp0 = kP.x + fT0 * kD.x;
    fTmp1 = kP.y + fT0 * kD.y;
    if (fTmp0 * fTmp0 + fTmp1 * fTmp1 <= fRadiusSqr)
    {
        code[iQuantity] = cyl_cap;
        afT[iQuantity++] = fT0 * fInvDLength;
    }

    fT1 = (-fHalfHeight - kP.z) * fInv;
    fTmp0 = kP.x + fT1 * kD.x;
    fTmp1 = kP.y + fT1 * kD.y;
    if (fTmp0 * fTmp0 + fTmp1 * fTmp1 <= fRadiusSqr)
    {
        code[iQuantity] = cyl_cap;
        afT[iQuantity++] = fT1 * fInvDLength;
    }

    if (iQuantity == 2)
    {
        // line intersects both top and bottom
        return 2; // both caps
    }

    // If iQuantity == 1, then line must intersect cylinder wall
    // somewhere between caps in a single point. This case is detected
    // in the following code that tests for intersection between line and
    // cylinder wall.

    fA = kD.x * kD.x + kD.y * kD.y;
    fB = kP.x * kD.x + kP.y * kD.y;
    fC = kP.x * kP.x + kP.y * kP.y - fRadiusSqr;
    fDiscr = fB * fB - fA * fC;
    if (fDiscr < 0.0f)
    {
        // line does not intersect cylinder wall
        // VERIFY( iQuantity == 0 );
        return 0;
    }
    else if (fDiscr > 0.0f)
    {
        fRoot = _sqrt(fDiscr);
        fInv = 1.0f / fA;
        fT = (-fB - fRoot) * fInv;
        if (fT0 <= fT1)
        {
            if (fT0 <= fT && fT <= fT1)
            {
                code[iQuantity] = cyl_wall;
                afT[iQuantity++] = fT * fInvDLength;
            }
        }
        else
        {
            if (fT1 <= fT && fT <= fT0)
            {
                code[iQuantity] = cyl_wall;
                afT[iQuantity++] = fT * fInvDLength;
            }
        }

        if (iQuantity == 2)
        {
            // Line intersects one of top/bottom of cylinder and once on
            // cylinder wall.
            return 2;
        }

        fT = (-fB + fRoot) * fInv;
        if (fT0 <= fT1)
        {
            if (fT0 <= fT && fT <= fT1)
            {
                code[iQuantity] = cyl_wall;
                afT[iQuantity++] = fT * fInvDLength;
            }
        }
        else
        {
            if (fT1 <= fT && fT <= fT0)
            {
                code[iQuantity] = cyl_wall;
                afT[iQuantity++] = fT * fInvDLength;
            }
        }
    }
    else
    {
        fT = -fB / fA;
        if (fT0 <= fT1)
        {
            if (fT0 <= fT && fT <= fT1)
            {
                code[iQuantity] = cyl_wall;
                afT[iQuantity++] = fT * fInvDLength;
            }
        }
        else
        {
            if (fT1 <= fT && fT <= fT0)
            {
                code[iQuantity] = cyl_wall;
                afT[iQuantity++] = fT * fInvDLength;
            }
        }
    }

    return iQuantity;
}

Fcylinder::ERP_Result Fcylinder::intersect(const Fvector3& start, const Fvector3& dir, float& dist) const
{
    float afT[2];
    ecode code[2];
    int cnt;
    if (0 != (cnt = intersect(start, dir, afT, code)))
    {
        bool o_inside = false;
        bool b_result = false;
        for (int k = 0; k < cnt; k++)
        {
            if (afT[k] < 0.f)
            {
                if (cnt == 2)
                    o_inside = true;
                continue;
            }
            if (afT[k] < dist)
            {
                dist = afT[k];
                b_result = true;
            }
        }
        return b_result ? (o_inside ? rpOriginInside : rpOriginOutside) : rpNone;
    }
    else
    {
        return rpNone;
    }
}
