#include "stdafx.h"
#include "xr_effgamma.h"

#if !defined(USE_DX9) && !defined(USE_OGL)

void CGammaControl::Update()
{
    if (HW.pDevice)
    {
        DXGI_GAMMA_CONTROL_CAPABILITIES GC;
        DXGI_GAMMA_CONTROL G;
        IDXGIOutput* pOutput;

        HRESULT hr = HW.m_pSwapChain->GetContainingOutput(&pOutput);
        // Метод выполнится успешно только в полноэкранном режиме.
        if (SUCCEEDED(hr))
        {
            hr = pOutput->GetGammaControlCapabilities(&GC);
            if (SUCCEEDED(hr))
            {
                GenLUT(GC, G);
                pOutput->SetGammaControl(&G);
            }
        }

        _RELEASE(pOutput);
    }
}

void CGammaControl::GenLUT(const DXGI_GAMMA_CONTROL_CAPABILITIES& GC, DXGI_GAMMA_CONTROL& G)
{
    DXGI_RGB Offset = {0, 0, 0};
    DXGI_RGB Scale = {1, 1, 1};
    G.Offset = Offset;
    G.Scale = Scale;

    float DeltaCV = (GC.MaxConvertedValue - GC.MinConvertedValue);

    float og = 1.f / (fGamma + EPS);
    float B = fBrightness / 2.f;
    float C = fContrast / 2.f;

    for (u32 i = 0; i < GC.NumGammaControlPoints; i++)
    {
        float c = (C + .5f) * powf(GC.ControlPointPositions[i], og) + (B - 0.5f) * 0.5f - C * 0.5f + 0.25f;

        c = GC.MinConvertedValue + c * DeltaCV;

        G.GammaCurve[i].Red = c * cBalance.r;
        G.GammaCurve[i].Green = c * cBalance.g;
        G.GammaCurve[i].Blue = c * cBalance.b;

        clamp(G.GammaCurve[i].Red, GC.MinConvertedValue, GC.MaxConvertedValue);
        clamp(G.GammaCurve[i].Green, GC.MinConvertedValue, GC.MaxConvertedValue);
        clamp(G.GammaCurve[i].Blue, GC.MinConvertedValue, GC.MaxConvertedValue);
    }
}

#else // !USE_DX9 && !USE_OGL

IC u16 clr2gamma(float c)
{
    int C = iFloor(c);
    clamp(C, 0, 65535);
    return u16(C);
}

void CGammaControl::Update()
{
    if (HW.pDevice)
    {
        D3DGAMMARAMP G;
        GenLUT(G);
        HW.pDevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &G);
    }
}
void CGammaControl::GenLUT(D3DGAMMARAMP& G)
{
    float og = 1.f / (fGamma + EPS);
    float B = fBrightness / 2.f;
    float C = fContrast / 2.f;
    for (int i = 0; i < 256; i++)
    {
        //		float	c		= 65535.f*(powf(float(i)/255, og) + fBrightness);
        float c = (C + .5f) * powf(i / 255.f, og) * 65535.f + (B - 0.5f) * 32768.f - C * 32768.f + 16384.f;
        G.red[i] = clr2gamma(c * cBalance.r);
        G.green[i] = clr2gamma(c * cBalance.g);
        G.blue[i] = clr2gamma(c * cBalance.b);
    }
}

#endif // !USE_DX9 && !USE_OGL
