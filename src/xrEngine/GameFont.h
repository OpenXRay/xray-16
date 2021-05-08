#pragma once

#include "xrEngine/IGameFont.hpp"
#include "xrCommon/xr_vector.h"
#include "xrCore/_vector3d.h"
#ifdef DEBUG
#include "xrCore/xrstring.h"
#endif

class IFontRender;

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
    const Fvector& GetCharTC(u16 c) { return TCMap[c]; }

public:
    CGameFont(pcstr section, u32 flags = 0);
    CGameFont(pcstr shader, pcstr texture, u32 flags = 0);
    virtual ~CGameFont();

    virtual void Initialize(pcstr shader, pcstr texture) override;
    virtual void SetColor(u32 C) override { dwCurrentColor = C; }
    virtual u32 GetColor() const override { return dwCurrentColor; }
    virtual void SetHeightI(float S) override;
    virtual void SetHeight(float S) override;
    virtual float GetHeight() const override { return fCurrentHeight; };
    virtual void SetInterval(float x, float y) override { vInterval.set(x, y); };
    virtual void SetInterval(const Fvector2& v) override { vInterval.set(v); };
    virtual void SetAligment(EAligment aligment) override { eCurrentAlignment = aligment; }
    virtual float SizeOf_(pcstr s) override;
    virtual float SizeOf_(const wchar_t* wsStr) override;
    virtual float SizeOf_(const char cChar); // only ANSII
    virtual float CurrentHeight_() override;
    virtual void OutSetI(float x, float y) override;
    virtual void OutSet(float x, float y) override;
    virtual Fvector2 GetPosition() const override { return {fCurrentX, fCurrentY}; }
    virtual void MasterOut(bool bCheckDevice, bool bUseCoords, bool bScaleCoords, bool bUseSkip, float _x, float _y,
        float _skip, pcstr fmt, va_list p) override;
    virtual u32 smart_strlen(pcstr S) override;
    virtual bool IsMultibyte() const override { return (uFlags & fsMultibyte); };
    virtual u16 SplitByWidth(u16* puBuffer, u16 uBufferSize, float fTargetWidth, pcstr pszText) override;
    virtual u16 GetCutLengthPos(float fTargetWidth, pcstr pszText) override;
    virtual void OutI(float _x, float _y, pcstr fmt, ...) override;
    virtual void Out(float _x, float _y, pcstr fmt, ...) override;
    virtual void OutNext(pcstr fmt, ...) override;
    virtual void OutNextVA(pcstr format, va_list args) override;
    virtual void OutSkip(float val = 1.f) override;
    virtual void OnRender() override;
    virtual void Clear() override { strings.clear(); }
#ifdef DEBUG
    shared_str m_font_name;
#endif
};
