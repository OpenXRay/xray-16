#ifndef TSS_H
#define TSS_H
#pragma once

#include "tss_def.h"

#if defined(USE_DX11) || defined(USE_DX12) || defined(USE_OGL)
enum XRDX11SAMPLERSTATETYPE
{
    XRDX11SAMP_ANISOTROPICFILTER = 256,
    XRDX11SAMP_COMPARISONFILTER,
    XRDX11SAMP_COMPARISONFUNC,
    XRDX11SAMP_MINLOD //	integer value. 0 - the most detailed level
};
enum XRDX11RENDERSTATETYPE
{
    XRDX11RS_ALPHATOCOVERAGE = 1024
};
#endif // !USE_DX9

class CSimulatorTSS
{
public:
    void Set(SimulatorStates& container, u32 S, u32 N, u32 V) { container.set_TSS(S, N, V); }

    void SetColor(SimulatorStates& container, u32 S, u32 A1, u32 OP, u32 A2)
    {
        Set(container, S, D3DTSS_COLOROP, OP);
        switch (OP)
        {
        case D3DTOP_DISABLE: break;
        case D3DTOP_SELECTARG1: Set(container, S, D3DTSS_COLORARG1, A1); break;
        case D3DTOP_SELECTARG2: Set(container, S, D3DTSS_COLORARG2, A2); break;
        default:
            Set(container, S, D3DTSS_COLORARG1, A1);
            Set(container, S, D3DTSS_COLORARG2, A2);
            break;
        }
    }

    void SetColor3(SimulatorStates& container, u32 S, u32 A1, u32 OP, u32 A2, u32 A3)
    {
        SetColor(container, S, A1, OP, A2);
        Set(container, S, D3DTSS_COLORARG0, A3);
    }

    void SetAlpha(SimulatorStates& container, u32 S, u32 A1, u32 OP, u32 A2)
    {
        Set(container, S, D3DTSS_ALPHAOP, OP);
        switch (OP)
        {
        case D3DTOP_DISABLE: break;
        case D3DTOP_SELECTARG1: Set(container, S, D3DTSS_ALPHAARG1, A1); break;
        case D3DTOP_SELECTARG2: Set(container, S, D3DTSS_ALPHAARG2, A2); break;
        default:
            Set(container, S, D3DTSS_ALPHAARG1, A1);
            Set(container, S, D3DTSS_ALPHAARG2, A2);
            break;
        }
    }

    void SetAlpha3(SimulatorStates& container, u32 S, u32 A1, u32 OP, u32 A2, u32 A3)
    {
        SetAlpha(container, S, A1, OP, A2);
        Set(container, S, D3DTSS_ALPHAARG0, A3);
    }
};

class CSimulatorRS
{
public:
    void Set(SimulatorStates& container, u32 N, u32 V)
    {
        //	Igor: XBox has render states 400 and hire
        // R_ASSERT(N<256);
        container.set_RS(N, V);
    }
};

class CSimulator
{
public:
    CSimulatorTSS TSS;
    CSimulatorRS RS;
    SimulatorStates container;

public:
    CSimulator() { Invalidate(); }
    void Invalidate() { container.clear(); }
    void SetTSS(u32 S, u32 N, u32 V) { TSS.Set(container, S, N, V); }
    void SetSAMP(u32 S, u32 N, u32 V) { container.set_SAMP(S, N, V); }
    void SetColor(u32 S, u32 a, u32 b, u32 c) { TSS.SetColor(container, S, a, b, c); }
    void SetColor3(u32 S, u32 a, u32 b, u32 c, u32 d) { TSS.SetColor3(container, S, a, b, c, d); }
    void SetAlpha(u32 S, u32 a, u32 b, u32 c) { TSS.SetAlpha(container, S, a, b, c); }
    void SetAlpha3(u32 S, u32 a, u32 b, u32 c, u32 d) { TSS.SetAlpha3(container, S, a, b, c, d); }
    void SetRS(u32 N, u32 V) { RS.Set(container, N, V); }
    SimulatorStates& GetContainer() { return container; }
};

#endif
