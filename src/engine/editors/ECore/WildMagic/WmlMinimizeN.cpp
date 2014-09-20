// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.
#include "stdafx.h"
#pragma hdrstop

#include "WmlMinimizeN.h"
#include "WmlMath.h"
using namespace Wml;

//----------------------------------------------------------------------------
template <class Real>
MinimizeN<Real>::MinimizeN (int iDimensions, Function oFunction,
    int iMaxLevel, int iMaxBracket, int iMaxIterations, void* pvData)
    :
    m_kMinimizer(LineFunction,iMaxLevel,iMaxBracket)
{
    assert( iDimensions >= 1 && oFunction );

    m_iDimensions = iDimensions;
    m_oFunction = oFunction;
    m_iMaxIterations = iMaxIterations;
    m_pvData = pvData;

    m_afTCurr = new Real[m_iDimensions];
    m_afTSave = new Real[m_iDimensions];
    m_afDirectionStorage = new Real[m_iDimensions*(m_iDimensions+1)];
    m_aafDirection = new Real*[m_iDimensions+1];
    for (int i = 0; i <= m_iDimensions; i++)
        m_aafDirection[i] = &m_afDirectionStorage[i*m_iDimensions];
    m_afDConj = m_aafDirection[m_iDimensions];

    m_afLineArg = new Real[m_iDimensions];
}
//----------------------------------------------------------------------------
template <class Real>
MinimizeN<Real>::~MinimizeN ()
{
    delete[] m_afTCurr;
    delete[] m_afTSave;
    delete[] m_afDirectionStorage;
    delete[] m_aafDirection;
    delete[] m_afLineArg;
}
//----------------------------------------------------------------------------
template <class Real>
int& MinimizeN<Real>::MaxLevel ()
{
    return m_kMinimizer.MaxLevel();
}
//----------------------------------------------------------------------------
template <class Real>
int& MinimizeN<Real>::MaxBracket ()
{
    return m_kMinimizer.MaxBracket();
}
//----------------------------------------------------------------------------
template <class Real>
void*& MinimizeN<Real>::UserData ()
{
    return m_pvData;
}
//----------------------------------------------------------------------------
template <class Real>
void MinimizeN<Real>::GetMinimum (const Real* afT0, const Real* afT1,
    const Real* afTInitial, Real* afTMin, Real& rfFMin)
{
    // for 1D function callback
    m_kMinimizer.UserData() = this;

    // initial guess
    int iQuantity = m_iDimensions*sizeof(Real);
    m_fFCurr = m_oFunction(afTInitial,m_pvData);
    memcpy(m_afTSave,afTInitial,iQuantity);
    memcpy(m_afTCurr,afTInitial,iQuantity);

    // initialize direction set to the standard Euclidean basis
    memset(m_afDirectionStorage,0,iQuantity*(m_iDimensions+1));
    int i;
    for (i = 0; i < m_iDimensions; i++)
        m_aafDirection[i][i] = (Real)1.0;

    Real fL0, fL1, fLMin;
    for (int iIter = 0; iIter < m_iMaxIterations; iIter++)
    {
        // find minimum in each direction and update current location
        for (i = 0; i < m_iDimensions; i++)
        {
            m_afDCurr = m_aafDirection[i];
            ComputeDomain(afT0,afT1,fL0,fL1);
            m_kMinimizer.GetMinimum(fL0,fL1,(Real)0.0,fLMin,m_fFCurr);
            for (int j = 0; j < m_iDimensions; j++)
                m_afTCurr[j] += fLMin*m_afDCurr[j];
        }

        // estimate a unit-length conjugate direction
        Real fLength = (Real)0.0;
        for (i = 0; i < m_iDimensions; i++)
        {
            m_afDConj[i] = m_afTCurr[i] - m_afTSave[i];
            fLength += m_afDConj[i]*m_afDConj[i];
        }

        const Real fEpsilon = (Real)1e-06;
        fLength = Math<Real>::Sqrt(fLength);
        if ( fLength < fEpsilon )
        {
            // New position did not change significantly from old one.
            // Should there be a better convergence criterion here?
            break;
        }

        Real fInvLength = ((Real)1.0)/fLength;
        for (i = 0; i < m_iDimensions; i++)
            m_afDConj[i] *= fInvLength;

        // minimize in conjugate direction
        m_afDCurr = m_afDConj;
        ComputeDomain(afT0,afT1,fL0,fL1);
        m_kMinimizer.GetMinimum(fL0,fL1,(Real)0.0,fLMin,m_fFCurr);
        for (i = 0; i < m_iDimensions; i++)
            m_afTCurr[i] += fLMin*m_afDCurr[i];

        // cycle the directions and add conjugate direction to set
        m_afDConj = m_aafDirection[0];
        for (i = 0; i < m_iDimensions; i++)
            m_aafDirection[i] = m_aafDirection[i+1];

        // set parameters for next pass
        memcpy(m_afTSave,m_afTCurr,iQuantity);
    }

    memcpy(afTMin,m_afTCurr,iQuantity);
    rfFMin = m_fFCurr;
}
//----------------------------------------------------------------------------
template <class Real>
void MinimizeN<Real>::ComputeDomain (const Real* afT0, const Real* afT1,
    Real& rfL0, Real& rfL1)
{
    rfL0 = -Math<Real>::MAX_REAL;
    rfL1 = +Math<Real>::MAX_REAL;

    for (int i = 0; i < m_iDimensions; i++)
    {
        Real fB0 = afT0[i] - m_afTCurr[i];
        Real fB1 = afT1[i] - m_afTCurr[i];
        Real fInv;
        if ( m_afDCurr[i] > (Real)0.0 )
        {
            // valid t-interval is [B0,B1]
            fInv = ((Real)1.0)/m_afDCurr[i];
            fB0 *= fInv;
            if ( fB0 > rfL0 )
                rfL0 = fB0;
            fB1 *= fInv;
            if ( fB1 < rfL1 )
                rfL1 = fB1;
        }
        else if ( m_afDCurr[i] < (Real)0.0 )
        {
            // valid t-interval is [B1,B0]
            fInv = ((Real)1.0)/m_afDCurr[i];
            fB0 *= fInv;
            if ( fB0 < rfL1 )
                rfL1 = fB0;
            fB1 *= fInv;
            if ( fB1 > rfL0 )
                rfL0 = fB1;
        }
    }

    // correction if numerical errors lead to values nearly zero
    if ( rfL0 > (Real)0.0 )
        rfL0 = (Real)0.0;
    if ( rfL1 < (Real)0.0 )
        rfL1 = (Real)0.0;
}
//----------------------------------------------------------------------------
template <class Real>
Real MinimizeN<Real>::LineFunction (Real fT, void* pvData)
{
    MinimizeN* pkThis = (MinimizeN*) pvData;

    for (int i = 0; i < pkThis->m_iDimensions; i++)
    {
        pkThis->m_afLineArg[i] = pkThis->m_afTCurr[i] +
            fT*pkThis->m_afDCurr[i];
    }

    Real fResult = pkThis->m_oFunction(pkThis->m_afLineArg,pkThis->m_pvData);
    return fResult;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
namespace Wml
{
template class WML_ITEM MinimizeN<float>;
template class WML_ITEM MinimizeN<double>;
}
//----------------------------------------------------------------------------
