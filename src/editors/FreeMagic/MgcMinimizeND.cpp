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

#include "MgcMinimizeND.h"
using namespace Mgc;

//----------------------------------------------------------------------------
MinimizeND::MinimizeND(int iDimensions, Function oF, int iMaxLevel,
                       int iMaxBracket, int iMaxIterations, void *pvUserData)
    : m_kMinimizer(LineFunction, iMaxLevel, iMaxBracket)
{
    assert(iDimensions >= 1 && oF);

    m_iDimensions = iDimensions;
    m_oF = oF;
    m_iMaxIterations = iMaxIterations;
    m_pvUserData = pvUserData;

    m_afTCurr = new Real[m_iDimensions];
    m_afTSave = new Real[m_iDimensions];
    m_afDirectionStorage = new Real[m_iDimensions * (m_iDimensions + 1)];
    m_aafDirection = new Real *[m_iDimensions + 1];
    for (int i = 0; i <= m_iDimensions; i++)
        m_aafDirection[i] = &m_afDirectionStorage[i * m_iDimensions];
    m_afDConj = m_aafDirection[m_iDimensions];

    m_afLineArg = new Real[m_iDimensions];
}
//----------------------------------------------------------------------------
MinimizeND::~MinimizeND()
{
    delete[] m_afTCurr;
    delete[] m_afTSave;
    delete[] m_afDirectionStorage;
    delete[] m_aafDirection;
    delete[] m_afLineArg;
}
//----------------------------------------------------------------------------
void MinimizeND::GetMinimum(const Real *afT0, const Real *afT1,
                            const Real *afTInitial, Real *afTMin, Real &rfFMin)
{
    // for 1D function callback
    m_kMinimizer.UserData() = this;

    // initial guess
    int iQuantity = m_iDimensions * sizeof(Real);
    m_fFCurr = m_oF(afTInitial, m_pvUserData);
    memcpy(m_afTSave, afTInitial, iQuantity);
    memcpy(m_afTCurr, afTInitial, iQuantity);

    // initialize direction set to the standard Euclidean basis
    memset(m_afDirectionStorage, 0, iQuantity * (m_iDimensions + 1));
    int i;
    for (i = 0; i < m_iDimensions; i++)
        m_aafDirection[i][i] = 1.0f;

    Real fL0, fL1, fLMin;
    for (int iIter = 0; iIter < m_iMaxIterations; iIter++)
    {
        // find minimum in each direction and update current location
        for (i = 0; i < m_iDimensions; i++)
        {
            m_afDCurr = m_aafDirection[i];
            ComputeDomain(afT0, afT1, fL0, fL1);
            m_kMinimizer.GetMinimum(fL0, fL1, 0.0f, fLMin, m_fFCurr);
            for (int j = 0; j < m_iDimensions; j++)
                m_afTCurr[j] += fLMin * m_afDCurr[j];
        }

        // estimate a unit-length conjugate direction
        Real fLength = 0.0f;
        for (i = 0; i < m_iDimensions; i++)
        {
            m_afDConj[i] = m_afTCurr[i] - m_afTSave[i];
            fLength += m_afDConj[i] * m_afDConj[i];
        }

        const Real fEpsilon = 1e-06f;
        fLength = Math::Sqrt(fLength);
        if (fLength < fEpsilon)
        {
            // New position did not change significantly from old one.
            // Should there be a better convergence criterion here?
            break;
        }

        Real fInvLength = 1.0f / fLength;
        for (i = 0; i < m_iDimensions; i++)
            m_afDConj[i] *= fInvLength;

        // minimize in conjugate direction
        m_afDCurr = m_afDConj;
        ComputeDomain(afT0, afT1, fL0, fL1);
        m_kMinimizer.GetMinimum(fL0, fL1, 0.0f, fLMin, m_fFCurr);
        for (i = 0; i < m_iDimensions; i++)
            m_afTCurr[i] += fLMin * m_afDCurr[i];

        // cycle the directions and add conjugate direction to set
        m_afDConj = m_aafDirection[0];
        for (i = 0; i < m_iDimensions; i++)
            m_aafDirection[i] = m_aafDirection[i + 1];

        // set parameters for next pass
        memcpy(m_afTSave, m_afTCurr, iQuantity);
    }

    memcpy(afTMin, m_afTCurr, iQuantity);
    rfFMin = m_fFCurr;
}
//----------------------------------------------------------------------------
void MinimizeND::ComputeDomain(const Real *afT0, const Real *afT1,
                               Real &rfL0, Real &rfL1)
{
    rfL0 = -Math::MAX_REAL;
    rfL1 = +Math::MAX_REAL;

    for (int i = 0; i < m_iDimensions; i++)
    {
        Real fB0 = afT0[i] - m_afTCurr[i];
        Real fB1 = afT1[i] - m_afTCurr[i];
        Real fInv;
        if (m_afDCurr[i] > 0.0f)
        {
            // valid t-interval is [B0,B1]
            fInv = 1.0f / m_afDCurr[i];
            fB0 *= fInv;
            if (fB0 > rfL0)
                rfL0 = fB0;
            fB1 *= fInv;
            if (fB1 < rfL1)
                rfL1 = fB1;
        }
        else if (m_afDCurr[i] < 0.0f)
        {
            // valid t-interval is [B1,B0]
            fInv = 1.0f / m_afDCurr[i];
            fB0 *= fInv;
            if (fB0 < rfL1)
                rfL1 = fB0;
            fB1 *= fInv;
            if (fB1 > rfL0)
                rfL0 = fB1;
        }
    }

    // correction if numerical errors lead to values nearly zero
    if (rfL0 > 0.0f)
        rfL0 = 0.0f;
    if (rfL1 < 0.0f)
        rfL1 = 0.0f;
}
//----------------------------------------------------------------------------
Real MinimizeND::LineFunction(Real fT, void *pvUserData)
{
    MinimizeND *pkThis = (MinimizeND *)pvUserData;

    for (int i = 0; i < pkThis->m_iDimensions; i++)
    {
        pkThis->m_afLineArg[i] = pkThis->m_afTCurr[i] +
                                 fT * pkThis->m_afDCurr[i];
    }

    Real fResult = pkThis->m_oF(pkThis->m_afLineArg, pkThis->m_pvUserData);
    return fResult;
}
//----------------------------------------------------------------------------
