#pragma once

#define TEMPLATE_SPECIALIZATION	template <int m_iDimensions>
#define _MinimizeND				MinimizeND<m_iDimensions>

TEMPLATE_SPECIALIZATION
inline int& _MinimizeND::MaxLevel ()
{
    return m_kMinimizer.MaxLevel();
}

TEMPLATE_SPECIALIZATION
inline int& _MinimizeND::MaxBracket ()
{
    return m_kMinimizer.MaxBracket();
}

TEMPLATE_SPECIALIZATION
inline void*& _MinimizeND::UserData ()
{
    return m_pvUserData;
}

TEMPLATE_SPECIALIZATION
_MinimizeND::MinimizeND (Function oF, int iMaxLevel,
    int iMaxBracket, int iMaxIterations, void* pvUserData)
    :
    m_kMinimizer(LineFunction,iMaxLevel,iMaxBracket)
{
    VERIFY( m_iDimensions >= 1 && oF );
    
	m_oF = oF;
    m_iMaxIterations = iMaxIterations;
    m_pvUserData = pvUserData;

    for (int i = 0; i <= m_iDimensions; i++)
        m_aafDirection[i] = &m_afDirectionStorage[i*m_iDimensions];
    m_afDConj = m_aafDirection[m_iDimensions];
}

TEMPLATE_SPECIALIZATION
void _MinimizeND::GetMinimum (const float* afT0, const float* afT1,
    const float* afTInitial, float* afTMin, float& rfFMin)
{
    // for 1D function callback
    m_kMinimizer.UserData() = this;

    // initial guess
    int iQuantity = m_iDimensions*sizeof(float);
    m_fFCurr = m_oF(afTInitial,m_pvUserData);
    memcpy(m_afTSave,afTInitial,iQuantity);
    memcpy(m_afTCurr,afTInitial,iQuantity);

    // initialize direction set to the standard Euclidean basis
    ZeroMemory(m_afDirectionStorage,iQuantity*(m_iDimensions+1));
    int i;
    for (i = 0; i < m_iDimensions; i++)
        m_aafDirection[i][i] = 1.0f;

    float fL0, fL1, fLMin;
    for (int iIter = 0; iIter < m_iMaxIterations; iIter++)
    {
        // find minimum in each direction and update current location
        for (i = 0; i < m_iDimensions; i++)
        {
            m_afDCurr = m_aafDirection[i];
            ComputeDomain(afT0,afT1,fL0,fL1);
            m_kMinimizer.GetMinimum(fL0,fL1,0.0f,fLMin,m_fFCurr);
            for (int j = 0; j < m_iDimensions; j++)
                m_afTCurr[j] += fLMin*m_afDCurr[j];
        }

        // estimate a unit-length conjugate direction
        float fLength = 0.0f;
        for (i = 0; i < m_iDimensions; i++)
        {
            m_afDConj[i] = m_afTCurr[i] - m_afTSave[i];
            fLength += m_afDConj[i]*m_afDConj[i];
        }

        const float fEpsilon = 1e-06f;
        fLength = _sqrt(fLength);
        if ( fLength < fEpsilon )
        {
            // New position did not change significantly from old one.
            // Should there be a better convergence criterion here?
            break;
        }

        float fInvLength = 1.0f/fLength;
        for (i = 0; i < m_iDimensions; i++)
            m_afDConj[i] *= fInvLength;

        // minimize in conjugate direction
        m_afDCurr = m_afDConj;
        ComputeDomain(afT0,afT1,fL0,fL1);
        m_kMinimizer.GetMinimum(fL0,fL1,0.0f,fLMin,m_fFCurr);
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

TEMPLATE_SPECIALIZATION
void _MinimizeND::ComputeDomain (const float* afT0, const float* afT1,
    float& rfL0, float& rfL1)
{
    rfL0 = -flt_max;
    rfL1 = +flt_max;

    for (int i = 0; i < m_iDimensions; i++)
    {
        float fB0 = afT0[i] - m_afTCurr[i];
        float fB1 = afT1[i] - m_afTCurr[i];
        float fInv;
        if ( m_afDCurr[i] > 0.0f )
        {
            // valid t-interval is [B0,B1]
            fInv = 1.0f/m_afDCurr[i];
            fB0 *= fInv;
            if ( fB0 > rfL0 )
                rfL0 = fB0;
            fB1 *= fInv;
            if ( fB1 < rfL1 )
                rfL1 = fB1;
        }
        else if ( m_afDCurr[i] < 0.0f )
        {
            // valid t-interval is [B1,B0]
            fInv = 1.0f/m_afDCurr[i];
            fB0 *= fInv;
            if ( fB0 < rfL1 )
                rfL1 = fB0;
            fB1 *= fInv;
            if ( fB1 > rfL0 )
                rfL0 = fB1;
        }
    }

    // correction if numerical errors lead to values nearly zero
    if ( rfL0 > 0.0f )
        rfL0 = 0.0f;
    if ( rfL1 < 0.0f )
        rfL1 = 0.0f;
}

TEMPLATE_SPECIALIZATION
float _MinimizeND::LineFunction (float fT, void* pvUserData)
{
    _MinimizeND* pkThis = (_MinimizeND*) pvUserData;

    for (int i = 0; i < m_iDimensions; i++)
    {
        pkThis->m_afLineArg[i] = pkThis->m_afTCurr[i] +
            fT*pkThis->m_afDCurr[i];
    }

    float fResult = pkThis->m_oF(pkThis->m_afLineArg,pkThis->m_pvUserData);
    return fResult;
}

#undef TEMPLATE_SPECIALIZATION
#undef _MinimizeND