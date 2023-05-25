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

#ifndef MGCMINIMIZEND_H
#define MGCMINIMIZEND_H

#include "MgcMinimize1D.h"

namespace Mgc
{

    class MAGICFM MinimizeND
    {
    public:
        typedef Real (*Function)(const Real *, void *);

        MinimizeND(int iDimensions, Function oF, int iMaxLevel, int iMaxBracket,
                   int iMaxIterations, void *pvUserData = 0);

        ~MinimizeND();

        int &MaxLevel();
        int &MaxBracket();
        void *&UserData();

        // find minimum on Cartesian-product domain
        void GetMinimum(const Real *afT0, const Real *afT1,
                        const Real *afTInitial, Real *afTMin, Real &rfFMin);

    protected:
        int m_iDimensions;
        Function m_oF;
        int m_iMaxIterations;
        void *m_pvUserData;
        Minimize1D m_kMinimizer;
        Real *m_afDirectionStorage;
        Real **m_aafDirection;
        Real *m_afDConj;
        Real *m_afDCurr;
        Real *m_afTSave;
        Real *m_afTCurr;
        Real m_fFCurr;
        Real *m_afLineArg;

        void ComputeDomain(const Real *afT0, const Real *afT1,
                           Real &rfL0, Real &rfL1);

        static Real LineFunction(Real fT, void *pvUserData);
    };

#include "MgcMinimizeND.inl"

} // namespace Mgc

#endif
