#ifndef GameFontH
#define GameFontH
#pragma once

#include "xrEngine/Engine.h"
#include "xrEngine/IGameFont.hpp"
#include "xrCore/Text/MbHelpers.h"
#include "Include/xrRender/FontRender.h"

class ENGINE_API CGameFont : public IGameFont
{
    friend class dxFontRender;

private:
    struct String
    {
        string1024 string;
        float x, y;
        float height;
        u32 c;
        EAligment align;
    };

protected:
    Fvector2 vHalfPixel;
    Ivector2 vTS;
    EAligment eCurrentAlignment;
    u32 dwCurrentColor;
    float fCurrentHeight;
    float fCurrentX, fCurrentY;
    Fvector2 vInterval;
    Fvector* TCMap;
    float fHeight;
    float fXStep;
    float fYStep;
    float fTCHeight;
    xr_vector<String> strings;
    IFontRender* pFontRender;
    u32 nNumChars;
    u32 uFlags;

protected:
    IC const Fvector& GetCharTC(u16 c) { return TCMap[c]; }
public:
    CGameFont(LPCSTR section, u32 flags = 0);
    CGameFont(LPCSTR shader, LPCSTR texture, u32 flags = 0);
    virtual ~CGameFont();

    virtual void Initialize(LPCSTR shader, LPCSTR texture) override;
    virtual void SetColor(u32 C) override { dwCurrentColor = C; }
    virtual u32 GetColor() const override { return dwCurrentColor; }
    virtual void SetHeightI(float S) override;
    virtual void SetHeight(float S) override;
    virtual float GetHeight() const override { return fCurrentHeight; };
    virtual void SetInterval(float x, float y) override { vInterval.set(x, y); };
    virtual void SetInterval(const Fvector2& v) override { vInterval.set(v); };
    virtual void SetAligment(EAligment aligment) override { eCurrentAlignment = aligment; }
    virtual float SizeOf_(LPCSTR s) override;
    virtual float SizeOf_(const wchar_t* wsStr) override;
    virtual float SizeOf_(const char cChar); // only ANSII
    virtual float CurrentHeight_() override;
    virtual void OutSetI(float x, float y) override;
    virtual void OutSet(float x, float y) override;
    virtual Fvector2 GetPosition() const override { return {fCurrentX, fCurrentY}; }
    virtual void MasterOut(BOOL bCheckDevice, BOOL bUseCoords, BOOL bScaleCoords, BOOL bUseSkip, float _x, float _y,
        float _skip, LPCSTR fmt, va_list p) override;
    virtual u32 smart_strlen(const char* S) override;
    virtual BOOL IsMultibyte() const override { return (uFlags & fsMultibyte); };
    virtual u16 SplitByWidth(u16* puBuffer, u16 uBufferSize, float fTargetWidth, const char* pszText) override;
    virtual u16 GetCutLengthPos(float fTargetWidth, const char* pszText) override;
    virtual void OutI(float _x, float _y, LPCSTR fmt, ...) override;
    virtual void Out(float _x, float _y, LPCSTR fmt, ...) override;
    virtual void OutNext(LPCSTR fmt, ...) override;
    virtual void OutNextVA(const char* format, va_list args) override;
    virtual void OutSkip(float val = 1.f) override;
    virtual void OnRender() override;
    virtual void Clear() override { strings.clear(); }
#ifdef DEBUG
    shared_str m_font_name;
#endif
};

#endif // _XR_GAMEFONT_H_
