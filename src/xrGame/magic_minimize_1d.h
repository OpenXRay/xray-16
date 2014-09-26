#pragma once

class Minimize1D
{
public:
    typedef float (*Function)(float,void*);

    Minimize1D (Function oF, int iMaxLevel, int iMaxBracket,
        void* pvUserData = 0);

    int& MaxLevel ();
    int& MaxBracket ();
    void*& UserData ();

    void GetMinimum (float fT0, float fT1, float fTInitial,
        float& rfTMin, float& rfFMin);

protected:
    Function m_oF;
    int m_iMaxLevel, m_iMaxBracket;
    float m_fTMin, m_fFMin;
    void* m_pvUserData;

    void GetMinimum (float fT0, float fF0, float fT1, float fF1, int iLevel);

    void GetMinimum (float fT0, float fF0, float fTm, float fFm, float fT1,
        float fF1, int iLevel);

    void GetBracketedMinimum (float fT0, float fF0, float fTm,
        float fFm, float fT1, float fF1, int iLevel);
};

#include "magic_minimize_1d_inline.h"