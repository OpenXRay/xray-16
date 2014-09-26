#pragma once

#include "magic_minimize_1d.h"

template <int m_iDimensions>
class MinimizeND
{
public:
    typedef float (*Function)(const float*,void*);

    MinimizeND (Function oF, int iMaxLevel, int iMaxBracket,
        int iMaxIterations, void* pvUserData = 0);

    int& MaxLevel ();
    int& MaxBracket ();
    void*& UserData ();

    // find minimum on Cartesian-product domain
    void GetMinimum (const float* afT0, const float* afT1,
        const float* afTInitial, float* afTMin, float& rfFMin);

protected:
    Function m_oF;
    int m_iMaxIterations;
    void* m_pvUserData;
    Minimize1D m_kMinimizer;
    float* m_afDConj;
    float* m_afDCurr;
    float m_fFCurr;

    float m_afTCurr[m_iDimensions];
    float m_afTSave[m_iDimensions];
    float m_afDirectionStorage[m_iDimensions*(m_iDimensions+1)];
    float *m_aafDirection[m_iDimensions+1];
    float m_afLineArg[m_iDimensions];

    void ComputeDomain (const float* afT0, const float* afT1,
        float& rfL0, float& rfL1);

    static float LineFunction (float fT, void* pvUserData);
};

#include "magic_minimize_nd_inline.h"