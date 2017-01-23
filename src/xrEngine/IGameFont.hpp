#pragma once
#include "xrCore/xrCore.h"

class IGameFont
{
    friend class dxFontRender;
public:
    enum EAligment
    {
        alLeft = 0,
        alRight,
        alCenter
    };
    enum
    {
        fsGradient = (1 << 0),
        fsDeviceIndependent = (1 << 1),
        fsValid = (1 << 2),
        fsMultibyte = (1 << 3),
        fsForceDWORD = u32(-1)
    };

public:
    virtual ~IGameFont() = 0;

    virtual void Initialize(LPCSTR shader, LPCSTR texture) = 0;
    virtual void SetColor(u32 C) = 0;
    virtual u32 GetColor() const = 0;
    virtual void SetHeightI(float S) = 0;
    virtual void SetHeight(float S) = 0;
    virtual float GetHeight() const = 0;
    virtual void SetInterval(float x, float y) = 0;
    virtual void SetInterval(const Fvector2& v) = 0;
    virtual void SetAligment(EAligment aligment) = 0;
    virtual float SizeOf_(LPCSTR s) = 0;
    virtual float SizeOf_(const wchar_t* wsStr) = 0;
    virtual float SizeOf_(const char cChar) = 0; // only ANSII
    virtual float CurrentHeight_() = 0;
    virtual void OutSetI(float x, float y) = 0;
    virtual void OutSet(float x, float y) = 0;
    virtual Fvector2 GetPosition() const = 0;
    virtual void MasterOut(BOOL bCheckDevice, BOOL bUseCoords, BOOL bScaleCoords, BOOL bUseSkip,
        float _x, float _y, float _skip, LPCSTR fmt, va_list p) = 0;
    virtual u32 smart_strlen(const char* S) = 0;
    virtual BOOL IsMultibyte() const = 0;
    virtual u16 SplitByWidth(u16* puBuffer, u16 uBufferSize, float fTargetWidth, const char* pszText) = 0;
    virtual u16 GetCutLengthPos(float fTargetWidth, const char* pszText) = 0;
    virtual void OutI(float _x, float _y, LPCSTR fmt, ...) = 0;
    virtual void Out(float _x, float _y, LPCSTR fmt, ...) = 0;
    virtual void OutNext(LPCSTR fmt, ...) = 0;
    virtual void OutNextVA(const char *format, va_list args) = 0;
    virtual void OutSkip(float val = 1.f) = 0;
    virtual void OnRender() = 0;
    virtual void Clear() = 0;
};

IC IGameFont::~IGameFont() {}
