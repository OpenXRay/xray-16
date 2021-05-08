#ifndef __XR_GAMMA_H__
#define __XR_GAMMA_H__

//-----------------------------------------------------------------------------------------------------------
// Gamma control
//-----------------------------------------------------------------------------------------------------------
class CGammaControl
{
    float fGamma;
    float fBrightness;
    float fContrast;
    Fcolor cBalance;

public:
    CGammaControl() : fGamma(1.f)
    {
        Brightness(1.f);
        Contrast(1.f);
        Balance(1.f, 1.f, 1.f);
    };

    void Balance(float _r, float _g, float _b) { cBalance.set(_r, _g, _b, 1); }
    void Balance(Fcolor& C) { Balance(C.r, C.g, C.b); }
    void Gamma(float G) { fGamma = G; }
    void Brightness(float B) { fBrightness = B; }
    void Contrast(float C) { fContrast = C; }
    void GetIP(float& G, float& B, float& C, Fcolor& Balance)
    {
        G = fGamma;
        B = fBrightness;
        C = fContrast;
        Balance.set(cBalance);
    }

    void Update();

private:
#if !defined(USE_DX9) && !defined(USE_OGL)
    void GenLUT(const DXGI_GAMMA_CONTROL_CAPABILITIES& GC, DXGI_GAMMA_CONTROL& G);
#else
    void GenLUT(D3DGAMMARAMP& G);
#endif
};

#endif
