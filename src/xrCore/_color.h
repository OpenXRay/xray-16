#pragma once

#include "_types.h"
#include "_std_extensions.h"
#include "math_constants.h"

constexpr s32 clamp_to_8bit(const s32 val) noexcept
{
    if (val < 0)
        return 0;
    if (val > 255)
        return 255;
    return val;
}

// maps unsigned 8 bits/channel to D3DCOLOR
constexpr u32 color_argb(u32 a, u32 r, u32 g, u32 b) noexcept
{
    return ((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
}

constexpr u32 color_rgba(u32 r, u32 g, u32 b, u32 a) noexcept
{
    return color_argb(a, r, g, b);
}

ICF u32 color_argb_f(f32 a, f32 r, f32 g, f32 b) noexcept
{
#if 0
    s32 _r = clampr(iFloor(r*255.f), 0, 255);
    s32 _g = clampr(iFloor(g*255.f), 0, 255);
    s32 _b = clampr(iFloor(b*255.f), 0, 255);
    s32 _a = clampr(iFloor(a*255.f), 0, 255);
#else
    s32 _r = clamp_to_8bit(iFloor(r*255.f));
    s32 _g = clamp_to_8bit(iFloor(g*255.f));
    s32 _b = clamp_to_8bit(iFloor(b*255.f));
    s32 _a = clamp_to_8bit(iFloor(a*255.f));
#endif
    return color_argb(_a, _r, _g, _b);
}

ICF u32 color_rgba_f(f32 r, f32 g, f32 b, f32 a) noexcept
{
    return color_argb_f(a, r, g, b);
}

constexpr u32 color_xrgb(u32 r, u32 g, u32 b) { return color_argb(0xff, r, g, b); }
constexpr u32 color_get_R(u32 rgba) noexcept { return ((rgba >> 16) & 0xff); }
constexpr u32 color_get_G(u32 rgba) noexcept { return ((rgba >> 8) & 0xff); }
constexpr u32 color_get_B(u32 rgba) noexcept { return (rgba & 0xff); }
constexpr u32 color_get_A(u32 rgba) noexcept { return (rgba >> 24); }
constexpr u32 subst_alpha(u32 rgba, u32 a) { return (rgba & ~color_rgba(0, 0, 0, 0xff)) | color_rgba(0, 0, 0, a); }
constexpr u32 bgr2rgb(u32 bgr) { return color_rgba(color_get_B(bgr), color_get_G(bgr), color_get_R(bgr), 0); }
constexpr u32 rgb2bgr(u32 rgb) { return bgr2rgb(rgb); }

struct Fcolor
{
    float r, g, b, a;

    Fcolor() noexcept = default;

    Fcolor(float _r, float _g, float _b, float _a) noexcept
        : r(_r), g(_g), b(_b), a(_a) {}

    Fcolor(u32 dw) noexcept
    {
        constexpr float f = 1.f / 255.f;
        a = f * float((dw >> 24) & 0xff);
        r = f * float((dw >> 16) & 0xff);
        g = f * float((dw >> 8) & 0xff);
        b = f * float((dw >> 0) & 0xff);
    }

    Fcolor& set(u32 dw) noexcept
    {
        constexpr float f = 1.f / 255.f;
        a = f * float((dw >> 24) & 0xff);
        r = f * float((dw >> 16) & 0xff);
        g = f * float((dw >> 8) & 0xff);
        b = f * float((dw >> 0) & 0xff);
        return *this;
    }

    Fcolor& operator=(u32 dw) noexcept { return set(dw); }

    Fcolor& set(float _r, float _g, float _b, float _a) noexcept
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
        return *this;
    }

    Fcolor& set(const Fcolor& rhs) noexcept
    {
        r = rhs.r;
        g = rhs.g;
        b = rhs.b;
        a = rhs.a;
        return *this;
    }
    u32 get() const noexcept { return color_rgba_f(r, g, b, a); }
    u32 get_windows() const noexcept // Get color as a Windows DWORD value.
    {
        u8 _a, _r, _g, _b;
        _a = u8(a*255.f);
        _r = u8(r*255.f);
        _g = u8(g*255.f);
        _b = u8(b*255.f);
        return (u32)(_a << 24) | (_b << 16) | (_g << 8) | _r;
    }
    Fcolor& set_windows(u32 dw) noexcept // Set color from a Windows DWORD color value.
    {
        const float f = 1.0f / 255.0f;
        a = f * (float)(u8)(dw >> 24);
        b = f * (float)(u8)(dw >> 16);
        g = f * (float)(u8)(dw >> 8);
        r = f * (float)(u8)(dw >> 0);
        return *this;
    }
    Fcolor& adjust_contrast(float f) noexcept // >1 - contrast will be increased
    {
        r = 0.5f + f * (r - 0.5f);
        g = 0.5f + f * (g - 0.5f);
        b = 0.5f + f * (b - 0.5f);
        return *this;
    }
    Fcolor& adjust_contrast(const Fcolor& in, float f) noexcept // >1 - contrast will be increased
    {
        r = 0.5f + f * (in.r - 0.5f);
        g = 0.5f + f * (in.g - 0.5f);
        b = 0.5f + f * (in.b - 0.5f);
        return *this;
    }
    Fcolor& adjust_saturation(float s) noexcept
    {
        // Approximate values for each component's contribution to luminance.
        // Based upon the NTSC standard described in ITU-R Recommendation BT.709.
        float grey = r * 0.2125f + g * 0.7154f + b * 0.0721f;
        r = grey + s * (r - grey);
        g = grey + s * (g - grey);
        b = grey + s * (b - grey);
        return *this;
    }
    Fcolor& adjust_saturation(const Fcolor& in, float s) noexcept
    {
        // Approximate values for each component's contribution to luminance.
        // Based upon the NTSC standard described in ITU-R Recommendation BT.709.
        float grey = in.r*0.2125f + in.g*0.7154f + in.b*0.0721f;
        r = grey + s * (in.r - grey);
        g = grey + s * (in.g - grey);
        b = grey + s * (in.b - grey);
        return *this;
    }
    Fcolor& modulate(Fcolor& in)               noexcept { r *= in.r; g *= in.g; b *= in.b; a *= in.a; return *this; }
    Fcolor& modulate(const Fcolor& in1, const Fcolor& in2) noexcept { r = in1.r*in2.r; g = in1.g*in2.g; b = in1.b*in2.b; a = in1.a*in2.a; return *this; }
    Fcolor& negative(const Fcolor& in)         noexcept { r = 1.0f - in.r; g = 1.0f - in.g; b = 1.0f - in.b; a = 1.0f - in.a; return *this; }
    Fcolor& negative()                         noexcept { r = 1.0f - r; g = 1.0f - g; b = 1.0f - b; a = 1.0f - a; return *this; }
    Fcolor& sub_rgb(float s)                   noexcept { r -= s; g -= s; b -= s; return *this; }
    Fcolor& add_rgb(float s)                   noexcept { r += s; g += s; b += s; return *this; }
    Fcolor& add_rgba(float s)                  noexcept { r += s; g += s; b += s; a += s; return *this; }
    Fcolor& mul_rgba(float s)                  noexcept { r *= s; g *= s; b *= s; a *= s; return *this; }
    Fcolor& mul_rgb(float s)                   noexcept { r *= s; g *= s; b *= s; return *this; }
    Fcolor& mul_rgba(const Fcolor& c, float s) noexcept { r = c.r*s; g = c.g*s; b = c.b*s; a = c.a*s; return *this; }
    Fcolor& mul_rgb(const Fcolor& c, float s)  noexcept { r = c.r*s; g = c.g*s; b = c.b*s; return *this; }

    // SQ magnitude
    float magnitude_sqr_rgb() const noexcept { return r * r + g * g + b * b;}
    // magnitude
    float magnitude_rgb()     const noexcept { return _sqrt(magnitude_sqr_rgb()); }
    float intensity() const noexcept
    {
        // XXX: Use the component percentages from adjust_saturation()?
        return (r + g + b) / 3.f;
    }
    // Normalize
    Fcolor& normalize_rgb()                noexcept { VERIFY(  magnitude_sqr_rgb() > EPS_S); return mul_rgb(   1.f /   magnitude_rgb()); }
    Fcolor& normalize_rgb(const Fcolor& c) noexcept { VERIFY(c.magnitude_sqr_rgb() > EPS_S); return mul_rgb(c, 1.f / c.magnitude_rgb()); }
    Fcolor& lerp(const Fcolor& c1, const Fcolor& c2, float t) noexcept
    {
        float invt = 1.f - t;
        r = c1.r*invt + c2.r*t;
        g = c1.g*invt + c2.g*t;
        b = c1.b*invt + c2.b*t;
        a = c1.a*invt + c2.a*t;
        return *this;
    }
    Fcolor& lerp(const Fcolor& c1, const Fcolor& c2, const Fcolor& c3, float t) noexcept
    {
        if (t>.5f)
            return lerp(c2, c3, t*2.f - 1.f);
        else
            return lerp(c1, c2, t*2.f);
    }
    bool similar_rgba(const Fcolor& v, float E = EPS_L) const noexcept { return _abs(r - v.r) < E && _abs(g - v.g) < E && _abs(b - v.b) < E && _abs(a - v.a) < E; }
    bool similar_rgb (const Fcolor& v, float E = EPS_L) const noexcept { return _abs(r - v.r) < E && _abs(g - v.g) < E && _abs(b - v.b) < E; }
};

IC bool _valid(const Fcolor& c) noexcept { return _valid(c.r) && _valid(c.g) && _valid(c.b) && _valid(c.a); }
