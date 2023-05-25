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

#ifndef MGCMINIMIZE1D_H
#define MGCMINIMIZE1D_H

#include "MgcMath.h"

namespace Mgc
{

    class MAGICFM Minimize1D
    {
    public:
        typedef Real (*Function)(Real, void *);

        Minimize1D(Function oF, int iMaxLevel, int iMaxBracket,
                   void *pvUserData = 0);

        int &MaxLevel();
        int &MaxBracket();
        void *&UserData();

        void GetMinimum(Real fT0, Real fT1, Real fTInitial,
                        Real &rfTMin, Real &rfFMin);

    protected:
        Function m_oF;
        int m_iMaxLevel, m_iMaxBracket;
        Real m_fTMin, m_fFMin;
        void *m_pvUserData;

        void GetMinimum(Real fT0, Real fF0, Real fT1, Real fF1, int iLevel);

        void GetMinimum(Real fT0, Real fF0, Real fTm, Real fFm, Real fT1,
                        Real fF1, int iLevel);

        void GetBracketedMinimum(Real fT0, Real fF0, Real fTm,
                                 Real fFm, Real fT1, Real fF1, int iLevel);
    };

#include "MgcMinimize1D.inl"

} // namespace Mgc

#endif
