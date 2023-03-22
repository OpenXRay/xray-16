#pragma once

#include "xrCore/xr_types.h"
#include "xrCore/_vector2.h"
#include "xrCore/Text/StringConversion.hpp"

class XR_NOVTABLE IGameFont
{
    friend class dxFontRender;

public:
    enum EAligment
    {
        alLeft = 0,
        alRight,
        alCenter
    };
    enum : u32
    {
        fsGradient = (1 << 0),
        fsDeviceIndependent = (1 << 1),
        fsValid = (1 << 2),
        fsMultibyte = (1 << 3),
    };

public:
    virtual ~IGameFont() = 0;

    virtual void Initialize(pcstr shader, pcstr texture) = 0;
    virtual void SetColor(u32 C) = 0;
    virtual u32 GetColor() const = 0;
    virtual void SetHeightI(float S) = 0;
    virtual void SetHeight(float S) = 0;
    virtual float GetHeight() const = 0;
    virtual void SetInterval(float x, float y) = 0;
    virtual void SetInterval(const Fvector2& v) = 0;
    virtual void SetAligment(EAligment aligment) = 0;
    virtual float SizeOf_(pcstr s) = 0;
    virtual float SizeOf_(const xr_wide_char* wsStr) = 0;
    virtual float SizeOf_(const char cChar) = 0; // only ANSI
    virtual float CurrentHeight_() = 0;
    virtual void OutSetI(float x, float y) = 0;
    virtual void OutSet(float x, float y) = 0;
    virtual Fvector2 GetPosition() const = 0;
    virtual void MasterOut(bool bCheckDevice, bool bUseCoords, bool bScaleCoords, bool bUseSkip, float _x, float _y,
        float _skip, pcstr fmt, va_list p) = 0;
    virtual u32 smart_strlen(pcstr S) = 0;
    virtual std::pair<u32, u32> get_actions_text_length(pcstr str) = 0; // returns count of actions plus total text size of all actions
    virtual bool IsMultibyte() const = 0;
    virtual u16 SplitByWidth(u16* puBuffer, u16 uBufferSize, float fTargetWidth, pcstr pszText) = 0;
    virtual u16 GetCutLengthPos(float fTargetWidth, pcstr pszText) = 0;
    virtual void OutI(float _x, float _y, pcstr fmt, ...) = 0;
    virtual void Out(float _x, float _y, pcstr fmt, ...) = 0;
    virtual void OutNext(pcstr fmt, ...) = 0;
    virtual void OutNextVA(pcstr format, va_list args) = 0;
    virtual void OutSkip(float val = 1.f) = 0;
    virtual void OnRender() = 0;
    virtual void Clear() = 0;
};

inline IGameFont::~IGameFont() = default;
