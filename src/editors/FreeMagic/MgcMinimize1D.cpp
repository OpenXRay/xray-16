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

#include "MgcMinimize1D.h"
using namespace Mgc;

//----------------------------------------------------------------------------
Minimize1D::Minimize1D(Function oF, int iMaxLevel, int iMaxBracket,
                       void *pvUserData)
{
    assert(oF);
    m_oF = oF;
    m_iMaxLevel = iMaxLevel;
    m_iMaxBracket = iMaxBracket;
    m_pvUserData = pvUserData;
}
//----------------------------------------------------------------------------
void Minimize1D::GetMinimum(Real fT0, Real fT1, Real fTInitial,
                            Real &rfTMin, Real &rfFMin)
{
    assert(fT0 <= fTInitial && fTInitial <= fT1);

    m_fTMin = Math::MAX_REAL;
    m_fFMin = Math::MAX_REAL;

    Real fF0 = m_oF(fT0, m_pvUserData);
    Real fFInitial = m_oF(fTInitial, m_pvUserData);
    Real fF1 = m_oF(fT1, m_pvUserData);

    GetMinimum(fT0, fF0, fTInitial, fFInitial, fT1, fF1, m_iMaxLevel);

    rfTMin = m_fTMin;
    rfFMin = m_fFMin;
}
//----------------------------------------------------------------------------
void Minimize1D::GetMinimum(Real fT0, Real fF0, Real fTm, Real fFm,
                            Real fT1, Real fF1, int iLevel)
{
    if (fF0 < m_fFMin)
    {
        m_fTMin = fT0;
        m_fFMin = fF0;
    }

    if (fFm < m_fFMin)
    {
        m_fTMin = fTm;
        m_fFMin = fFm;
    }

    if (fF1 < m_fFMin)
    {
        m_fTMin = fT1;
        m_fFMin = fF1;
    }

    if (iLevel-- == 0)
        return;

    if ((fT1 - fTm) * (fF0 - fFm) > (fTm - fT0) * (fFm - fF1))
    {
        // quadratic fit has positive second derivative at midpoint

        if (fF1 > fF0)
        {
            if (fFm >= fF0)
            {
                // increasing, repeat on [t0,tm]
                GetMinimum(fT0, fF0, fTm, fFm, iLevel);
            }
            else
            {
                // not monotonic, have a bracket
                GetBracketedMinimum(fT0, fF0, fTm, fFm, fT1, fF1, iLevel);
            }
        }
        else if (fF1 < fF0)
        {
            if (fFm >= fF1)
            {
                // decreasing, repeat on [tm,t1]
                GetMinimum(fTm, fFm, fT1, fF1, iLevel);
            }
            else
            {
                // not monotonic, have a bracket
                GetBracketedMinimum(fT0, fF0, fTm, fFm, fT1, fF1, iLevel);
            }
        }
        else
        {
            // constant, repeat on [t0,tm] and [tm,t1]
            GetMinimum(fT0, fF0, fTm, fFm, iLevel);
            GetMinimum(fTm, fFm, fT1, fF1, iLevel);
        }
    }
    else
    {
        // quadratic fit has nonpositive second derivative at midpoint

        if (fF1 > fF0)
        {
            // repeat on [t0,tm]
            GetMinimum(fT0, fF0, fTm, fFm, iLevel);
        }
        else if (fF1 < fF0)
        {
            // repeat on [tm,t1]
            GetMinimum(fTm, fFm, fT1, fF1, iLevel);
        }
        else
        {
            // repeat on [t0,tm] and [tm,t1]
            GetMinimum(fT0, fF0, fTm, fFm, iLevel);
            GetMinimum(fTm, fFm, fT1, fF1, iLevel);
        }
    }
}
//----------------------------------------------------------------------------
void Minimize1D::GetMinimum(Real fT0, Real fF0, Real fT1, Real fF1,
                            int iLevel)
{
    if (fF0 < m_fFMin)
    {
        m_fTMin = fT0;
        m_fFMin = fF0;
    }

    if (fF1 < m_fFMin)
    {
        m_fTMin = fT1;
        m_fFMin = fF1;
    }

    if (iLevel-- == 0)
        return;

    Real fTm = 0.5f * (fT0 + fT1);
    Real fFm = m_oF(fTm, m_pvUserData);

    if (fF0 - 2.0f * fFm + fF1 > 0.0f)
    {
        // quadratic fit has positive second derivative at midpoint

        if (fF1 > fF0)
        {
            if (fFm >= fF0)
            {
                // increasing, repeat on [t0,tm]
                GetMinimum(fT0, fF0, fTm, fFm, iLevel);
            }
            else
            {
                // not monotonic, have a bracket
                GetBracketedMinimum(fT0, fF0, fTm, fFm, fT1, fF1, iLevel);
            }
        }
        else if (fF1 < fF0)
        {
            if (fFm >= fF1)
            {
                // decreasing, repeat on [tm,t1]
                GetMinimum(fTm, fFm, fT1, fF1, iLevel);
            }
            else
            {
                // not monotonic, have a bracket
                GetBracketedMinimum(fT0, fF0, fTm, fFm, fT1, fF1, iLevel);
            }
        }
        else
        {
            // constant, repeat on [t0,tm] and [tm,t1]
            GetMinimum(fT0, fF0, fTm, fFm, iLevel);
            GetMinimum(fTm, fFm, fT1, fF1, iLevel);
        }
    }
    else
    {
        // quadratic fit has nonpositive second derivative at midpoint

        if (fF1 > fF0)
        {
            // repeat on [t0,tm]
            GetMinimum(fT0, fF0, fTm, fFm, iLevel);
        }
        else if (fF1 < fF0)
        {
            // repeat on [tm,t1]
            GetMinimum(fTm, fFm, fT1, fF1, iLevel);
        }
        else
        {
            // repeat on [t0,tm] and [tm,t1]
            GetMinimum(fT0, fF0, fTm, fFm, iLevel);
            GetMinimum(fTm, fFm, fT1, fF1, iLevel);
        }
    }
}
//----------------------------------------------------------------------------
void Minimize1D::GetBracketedMinimum(Real fT0, Real fF0, Real fTm,
                                     Real fFm, Real fT1, Real fF1, int iLevel)
{
    for (int i = 0; i < m_iMaxBracket; i++)
    {
        // update minimum value
        if (fFm < m_fFMin)
        {
            m_fTMin = fTm;
            m_fFMin = fFm;
        }

        // test for convergence
        const Real fEps = 1e-08f, fTol = 1e-04f;
        if (Math::FAbs(fT1 - fT0) <= 2.0f * fTol * Math::FAbs(fTm) + fEps)
            break;

        // compute vertex of interpolating parabola
        Real fDT0 = fT0 - fTm, fDT1 = fT1 - fTm;
        Real fDF0 = fF0 - fFm, fDF1 = fF1 - fFm;
        Real fTmp0 = fDT0 * fDF1, fTmp1 = fDT1 * fDF0;
        Real fDenom = fTmp1 - fTmp0;
        if (Math::FAbs(fDenom) < fEps)
            return;

        Real fTv = fTm + 0.5f * (fDT1 * fTmp1 - fDT0 * fTmp0) / fDenom;
        assert(fT0 <= fTv && fTv <= fT1);
        Real fFv = m_oF(fTv, m_pvUserData);

        if (fTv < fTm)
        {
            if (fFv < fFm)
            {
                fT1 = fTm;
                fF1 = fFm;
                fTm = fTv;
                fFm = fFv;
            }
            else
            {
                fT0 = fTv;
                fF0 = fFv;
            }
        }
        else if (fTv > fTm)
        {
            if (fFv < fFm)
            {
                fT0 = fTm;
                fF0 = fFm;
                fTm = fTv;
                fFm = fFv;
            }
            else
            {
                fT1 = fTv;
                fF1 = fFv;
            }
        }
        else
        {
            // vertex of parabola is already at middle sample point
            GetMinimum(fT0, fF0, fTm, fFm, iLevel);
            GetMinimum(fTm, fFm, fT1, fF1, iLevel);
        }
    }
}
//----------------------------------------------------------------------------
