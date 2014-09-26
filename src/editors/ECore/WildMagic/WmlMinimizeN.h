// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLMINIMIZENH
#define WMLMINIMIZENH

#include "WmlMinimize1.h"

namespace Wml
{

template <class Real>
class WML_ITEM MinimizeN
{
public:
    typedef Real (*Function)(const Real*,void*);

    MinimizeN (int iDimensions, Function oFunction, int iMaxLevel,
        int iMaxBracket, int iMaxIterations, void* pvData = 0);

    ~MinimizeN ();

    int& MaxLevel ();
    int& MaxBracket ();
    void*& UserData ();

    // find minimum on Cartesian-product domain
    void GetMinimum (const Real* afT0, const Real* afT1,
        const Real* afTInitial, Real* afTMin, Real& rfFMin);

protected:
    int m_iDimensions;
    Function m_oFunction;
    int m_iMaxIterations;
    void* m_pvData;
    Minimize1<Real> m_kMinimizer;
    Real* m_afDirectionStorage;
    Real** m_aafDirection;
    Real* m_afDConj;
    Real* m_afDCurr;
    Real* m_afTSave;
    Real* m_afTCurr;
    Real m_fFCurr;
    Real* m_afLineArg;

    void ComputeDomain (const Real* afT0, const Real* afT1, Real& rfL0,
        Real& rfL1);

    static Real LineFunction (Real fT, void* pvData);
};

typedef MinimizeN<float> MinimizeNf;
typedef MinimizeN<double> MinimizeNd;

}

#endif
