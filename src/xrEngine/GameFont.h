#pragma once

#include "xrEngine/IGameFont.hpp"
#include "xrCommon/xr_vector.h"
#include "xrCore/_vector3d.h"
#include "xrCore/Text/Utf8Utils.hpp"
#include "xrCore/Threading/Lock.hpp"
#include "xrCore/Threading/ScopeLock.hpp"

struct FT_FaceRec_;
using FT_Face = FT_FaceRec_*;

namespace xray::render
{
namespace render_r4 { class dxFontRender; }
namespace render_gl { class dxFontRender; }
namespace render_r2 { class dxFontRender; }
namespace render_r3 { class dxFontRender; }
} // namespace xray::render

class IFontRender;

class ENGINE_API CGameFont final : public IGameFont
{
    friend class xray::render::render_r4::dxFontRender;
    friend class xray::render::render_gl::dxFontRender;
    friend class xray::render::render_r2::dxFontRender;
    friend class xray::render::render_r3::dxFontRender;

public:
    enum EGradientMode
    {
        gm_vert = 0,
        gm_horz = 1,
        gm_back = 2,
        gm_down = 3,
        gm_count
    };

    struct ABC
    {
        int abcA;
        u32 abcB;
        int abcC;
    };

    struct Glyph
    {
        RECT TextureCoord;
        ABC Abc;
        int yOffset;
    };

private:
    struct String
    {
        string2048 string;
        xr_string string_utf8;
        float x, y;
        float height;
        u32 c;
        EAligment align;
        bool gradient;
        EGradientMode gradientMode;
        u32 gradientColor;
    };

    struct BaseData
    {
        bool OpenType = false;
        bool HasUnicodeCharmap = false; // true if FreeType selected a Unicode cmap
        bool IsBitmap = false;          // true: legacy bitmap font (.dds + .ini), no FreeType atlas
        u16 Size = 14;
        const char* Name = nullptr;      // font family file name (e.g. "arial")
        const char* Shader = nullptr;
        const char* Style = nullptr;
        const char* TextureName = nullptr; // unique atlas texture name (e.g. section "font_default")
    };

    BaseData Data;

    float fCurrentHeight = 0.0f;
    float fCurrentX = 0.0f;
    float fCurrentY = 0.0f;
    bool fGradientEnabled = false;
    EGradientMode fGradientMode = gm_vert;

    u8 uFlags = 0;
    u32 dwCurrentColor = 0;
    u32 dwGradientColor = 0;

    EAligment eCurrentAlignment = alLeft;
    Fvector2 vInterval{ 1.f, 1.f };

    Lock s_cs;
    xr_vector<String> strings;
    IFontRender* pFontRender = nullptr;

public:
    CGameFont(pcstr section, u8 flags = 0);
    virtual ~CGameFont();

    void ReInit();

    void SetColor(u32 C) override { dwCurrentColor = C; }
    u32 GetColor() const override { return dwCurrentColor; }
    void SetGradientColor(u32 C) { dwGradientColor = C; }

    void SetHeight(float S) override;
    void SetHeightI(float S) override;
    float GetHeight() const override { return fCurrentHeight; }

    void SetAligment(EAligment aligment) override { eCurrentAlignment = aligment; }

    void SetInterval(float x, float y) override { vInterval.set(x, y); }
    void SetInterval(const Fvector2& v) override { vInterval.set(v); }

    float SizeOf_(pcstr s) override;

    float CurrentHeight_() override;

    void OutSetI(float x, float y) override;
    void OutSet(float x, float y) override;
    Fvector2 GetPosition() const override { return { fCurrentX, fCurrentY }; }

    void MasterOut(bool bCheckDevice, bool bUseCoords, bool bScaleCoords, bool bUseSkip,
        float _x, float _y, float _skip, pcstr fmt, va_list p) override;

    u32 smart_strlen(pcstr S) override;

    bool IsMultibyte() const override { return (uFlags & fsMultibyte) != 0; }

    u16 SplitByWidth(u16* puBuffer, u16 uBufferSize, float fTargetWidth, pcstr pszText) override;
    u16 GetCutLengthPos(float fTargetWidth, pcstr pszText) override;

    void OutI(float _x, float _y, pcstr fmt, ...) override;
    void Out(float _x, float _y, pcstr fmt, ...) override;
    void OutNext(pcstr fmt, ...) override;
    void OutNextVA(pcstr format, va_list args) override;
    void OutSkip(float val = 1.f) override;

    void OnRender() override;
    void Clear() override { strings.clear(); }

    inline u32 GetSize() const { return Data.Size; }
    inline float GetLetterSpacing() const { return LetterSpacing; }
    inline void SetLetterSpacing(float spacing) { LetterSpacing = spacing; }
    inline float GetLineSpacing() const { return LineSpacing; }
    inline void SetLineSpacing(float spacing) { LineSpacing = spacing; }

    const Glyph* GetGlyphInfo(int ch);
    float WidthOf(int ch);
    float WidthOf(pcstr str);
    float SizeOf_(int cChar); // internal single-codepoint width, used by rendering pipeline

private:
    float LetterSpacing = 0.f;
    float LineSpacing = 1.f;

    struct StyleBits
    {
        u32 bold : 1;
        u32 italic : 1;
        u32 underline : 1;
        u32 strike : 1;
    } Style{};

    FT_Face OurFont = nullptr;
    IReader* m_fontReader = nullptr;
    xr_map<int, Glyph> GlyphData;

    void Prepare(pcstr name, pcstr shader, pcstr style, u32 size);
    void Initialize2(pcstr name, pcstr shader, pcstr style, u32 size);
    // Legacy bitmap-font fallback: loads a .dds + .ini font, converts CP1251-keyed
    // glyph metrics into the UTF-8 GlyphData map, and reuses the UTF-8 renderer.
    // Invoked from Initialize2 when no TTF/OTF fonts are present in $game_fonts$.
    void InitializeLegacy(pcstr name, pcstr shader, pcstr style, u32 size);

    static bool bFreetypeInitialized;
    static void InitializeFreetype();

#ifdef DEBUG
public:
    shared_str m_font_name;
#endif
};

// Global text scale multiplier (controlled by the `ui_text_scale` console variable).
// Applied on the draw path (WidthOf/CurrentHeight_), so changes take effect
// immediately in runtime without rebuilding the FreeType font atlas.
ENGINE_API extern float g_text_scale;

// Font renderer selector (controlled by the `r_font_legacy` console variable).
//   0 (default) = FreeType TTF/OTF rendering (UTF-8 native).
//   1           = legacy X-Ray bitmap fonts (.dds + .ini), CP1251 metrics
//                 converted to Unicode codepoints on load.
// Changes apply on the next font (re)initialization (e.g. vid_restart / UI reset).
ENGINE_API extern int g_font_legacy;
