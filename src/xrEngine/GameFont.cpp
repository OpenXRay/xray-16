#include "stdafx.h"
#pragma hdrstop

#if defined(XR_PLATFORM_WINDOWS)
#include <Windows.h>
#endif

#include "GameFont.h"
#include "xr_level_controller.h"
#include "xrCore/Text/StringConversion.hpp"
#include "Render.h"
#include "StringTable/StringTable.h"
#include "Include/xrRender/RenderFactory.h"
#include "Include/xrRender/FontRender.h"

#include <freetype/freetype.h>
#include <freetype/ftfntfmt.h>

#include <sstream>

extern ENGINE_API bool g_bRendering;

// Global text scale multiplier (cvar `ui_text_scale`). Multiplies the width and
// height returned by the font on the draw path so that the whole in-game text can
// be resized at runtime without rebuilding the FreeType atlas.
ENGINE_API float g_text_scale = 0.85f;

// Font renderer selector (cvar `r_font_legacy`). 0 = FreeType TTF, 1 =
// legacy X-Ray bitmap fonts. Read on each font (re)initialization.
ENGINE_API int g_font_legacy = 0;

FT_Library FreetypeLib = nullptr;
bool CGameFont::bFreetypeInitialized = false;

namespace
{
    constexpr u32 TextureDimension = 4096;
} // namespace

void CGameFont::InitializeFreetype()
{
    FT_Error Error = FT_Init_FreeType(&FreetypeLib);
    R_ASSERT2(Error == 0, "FreeType initialize failed");
}

static inline float DI2PX(float x)
{
    return float(iFloor((x + 1) * float(Device.dwWidth) * 0.5f));
}
static inline float DI2PY(float y)
{
    return float(iFloor((y + 1) * float(Device.dwHeight) * 0.5f));
}

CGameFont::CGameFont(pcstr section, u8 flags) : LineSpacing(1.f), LetterSpacing(0.f)
{
#ifdef DEBUG
    m_font_name = section;
    Msg("* Init font %s", section);
#endif

    uFlags = flags | fsMultibyte;

    // Read font name
    if (pSettings->line_exist(section, "name"))
    {
        xr_string nameStr = pSettings->r_string(section, "name");
        XRay::Utf8::ToLowerAscii(nameStr.data());
        Data.Name = xr_strdup(nameStr.c_str());
    }

    if (!Data.Name || !xr_strlen(Data.Name))
        Data.Name = xr_strdup(section);

    Data.TextureName = xr_strdup(section);

    // Set font shader and style
    Data.Shader = xr_strdup(pSettings->r_string(section, "shader"));

    Data.Style = nullptr;
    if (pSettings->line_exist(section, "style"))
        Data.Style = xr_strdup(pSettings->r_string(section, "style"));

    // Read font size
    Data.Size = 14;
    if (pSettings->line_exist(section, "size"))
        Data.Size = (u16)std::min<u32>(pSettings->r_u32(section, "size"), u16(-1));

    if (pSettings->line_exist(section, "opentype"))
        Data.OpenType = pSettings->r_bool(section, "opentype");

    if (pSettings->line_exist(section, "letter_spacing"))
        SetLetterSpacing(pSettings->r_float(section, "letter_spacing"));

    if (pSettings->line_exist(section, "line_spacing"))
        SetLineSpacing(pSettings->r_float(section, "line_spacing"));

    // Init
    pFontRender = GEnv.RenderFactory->CreateFontRender();
    Prepare(Data.Name, Data.Shader, Data.Style, Data.Size);
}

CGameFont::~CGameFont()
{
    // Shading
    if (OurFont)
    {
        FT_Done_Face(OurFont);
        OurFont = nullptr;
    }

    if (m_fontReader)
    {
        FS.r_close(m_fontReader);
        m_fontReader = nullptr;
    }

    GEnv.RenderFactory->DestroyFontRender(pFontRender);
    pFontRender = nullptr;

    xr_free(Data.Shader);
    xr_free(Data.Style);
    xr_free(Data.Name);
    xr_free(Data.TextureName);
}

void CGameFont::ReInit()
{
    Prepare(Data.Name, Data.Shader, Data.Style, Data.Size);
}

void CGameFont::Prepare(pcstr name, pcstr shader, pcstr style, u32 size)
{
    Initialize2(name, shader, style, size);
}

static xr_vector<xr_string> split(const xr_string& s, char delim)
{
    xr_vector<xr_string> elems;
    std::stringstream ss(s.c_str());
    xr_string item;
    while (std::getline(ss, item, delim))
        elems.push_back(item);
    return std::move(elems);
}

// Resolve the legacy bitmap font texture name. Priority:
//   1. Explicit "legacy_texture" field in the section (fonts.ltx).
//   2. Hardcoded section->texture table (old engine naming, no common pattern).
//   3. Heuristic "ui_font_<lower(name)>_<resSuffix>" as a last resort.
// The .dds/.ini files live in $game_textures$ alongside the old engine layout.
static void resolve_legacy_texture(pcstr section, pcstr name, string_path& outTexture)
{
    // 1. Explicit override in fonts.ltx (recommended for any new section).
    pcstr explicitTex = READ_IF_EXISTS(pSettings, r_string, section, "legacy_texture", nullptr);
    if (explicitTex && explicitTex[0])
    {
        xr_strcpy(outTexture, sizeof(outTexture), explicitTex);
        return;
    }

    // 2. Hardcoded table for the GW sections. Old font texture names have no
    //    common naming pattern (arial_14_1024, graff_19_1024, letter_16_1024,
    //    hud_01, console_02), so a table is the only reliable mapping.
    //    Paths are relative to $game_textures$ (the old engine stored fonts in textures\ui\).
    struct SectionToTex { pcstr section; pcstr tex; };
    static const SectionToTex table[] =
    {
        { "stat_font",                      "ui\\ui_font_hud_01"      },
        { "hud_font_medium",                "ui\\ui_font_hud_02"      },
        { "hud_font_di",                    "ui\\ui_font_console_02"  },
        { "hud_font_di2",                   "ui\\ui_font_console_02"  },
        { "ui_font_arial_14",               "ui\\ui_font_arial_14_1024"  },
        { "ui_font_graffiti19_russian",     "ui\\ui_font_graff_19_1024"  },
        { "ui_font_graffiti22_russian",     "ui\\ui_font_graff_22_1024"  },
        { "ui_font_graff_32",               "ui\\ui_font_graff_32_1024"  },
        { "ui_font_graff_40",               "ui\\ui_font_graff_40_1024"  },
        { "ui_font_graff_50",               "ui\\ui_font_graff_50_1024"  },
        { "ui_font_letterica16_russian",    "ui\\ui_font_letter_16_1024" },
        { "ui_font_letterica18_russian",    "ui\\ui_font_letter_18_1024" },
        { "ui_font_letter_25",              "ui\\ui_font_letter_25_1024" },
        { "ui_font_letterica16",            "ui\\ui_font_letter_16_1024" },
        { "ui_font_letterica18",            "ui\\ui_font_letter_18_1024" },
        { "ui_font_letterica25",            "ui\\ui_font_letter_25_1024" },
        { "ui_font_graffiti19",             "ui\\ui_font_graff_19_1024"  },
        { "ui_font_graffiti22",             "ui\\ui_font_graff_22_1024"  },
    };
    for (const auto& entry : table)
    {
        if (xr_strcmp(section, entry.section) == 0)
        {
            xr_strcpy(outTexture, sizeof(outTexture), entry.tex);
            return;
        }
    }

    // 3. Heuristic fallback (may not match real files; emit a hint to the log).
    xr_string base = "ui\\ui_font_";
    base += name;
    XRay::Utf8::ToLowerAscii(base.data());
    base += "_1024";
    xr_strcpy(outTexture, sizeof(outTexture), base.c_str());
    Msg("~ resolve_legacy_texture: section '%s' not in table, heuristic '%s'", section, outTexture);
}

void CGameFont::InitializeLegacy(pcstr name, pcstr shader, pcstr /*style*/, u32 size)
{
    Data.IsBitmap = true;
    Data.HasUnicodeCharmap = false; // metrics are CP1251-keyed; we translate to Unicode
    Data.OpenType = false;

    GlyphData.clear();
    Data.Size = static_cast<u16>(size);

    // 1. Resolve the texture + .ini path (located in $game_textures$, old layout).
    string_path textureName;
    resolve_legacy_texture(Data.TextureName ? Data.TextureName : name, name, textureName);

    string_path iniPath;
    if (!FS.exist(iniPath, "$game_textures$", textureName, ".ini"))
    {
        // Hard failure: do NOT silently return, otherwise pTexture stays null
        // and the renderer crashes with a null dereference. Give a clear message.
        R_ASSERT3(false, "r_font_legacy: no legacy font .ini found for section. "
            "Check fonts.ltx 'legacy_texture' or disable r_font_legacy.",
            Data.TextureName ? Data.TextureName : name);
    }

    Msg("~ Legacy font '%s' -> texture '%s', ini '%s'", name, textureName, iniPath);

    // 2. Read the .ini glyph metrics. The old engine supported four .ini formats;
    //    we handle all of them and convert each into the UTF-8 GlyphData map.
    //    All formats are keyed by CP1251 byte (0..255), so every glyph is stored
    //    under its Unicode codepoint via CP1251ByteToCodepoint().
    CInifile* ini = CInifile::Create(iniPath, true); // read-only

    // Helper: store a glyph under its Unicode codepoint.
    //   - TextureCoord: pixel rect of the glyph bitmap in the .dds atlas.
    //   - advance: total horizontal step to the next glyph. In the old engine this
    //     was the per-glyph width (right-left); the new renderer derives the step
    //     from abcA+abcB+abcC, so we put the bitmap width into abcB and the
    //     remaining advance into abcC.
    // NOTE: the space glyph MUST be stored too — otherwise it has no advance and
    // words run together. The old engine rendered space as an (empty) atlas cell
    // with a real advance; we do the same.
    auto addGlyph = [&](u8 cp1251Byte, LONG left, LONG top, LONG right, LONG bottom, float advance)
    {
        const int codepoint = static_cast<int>(XRay::Utf8::CP1251ByteToCodepoint(cp1251Byte));
        const LONG bitmapWidth = std::max<LONG>(0, right - left);
        Glyph glyph{};
        glyph.TextureCoord = { left, top, right, bottom };
        glyph.Abc.abcA = 0;
        glyph.Abc.abcB = static_cast<u32>(bitmapWidth);
        // post-bearing = advance - bitmapWidth, so abcA+abcB+abcC == advance.
        glyph.Abc.abcC = static_cast<int>(std::max(0.0f, advance - bitmapWidth));
        glyph.yOffset = 0;
        GlyphData[codepoint] = glyph;
    };

    float fontHeight = 16.0f; // sensible default if no section provides it
    char keyBuf[16];

    if (ini->section_exist("mb_symbol_coords"))
    {
        // Multibyte format: 5-digit keys (codepoint index, 0..0xFFFF). Each row is
        // "u, v, right" (r_fvector3). Here the key IS the codepoint directly (no
        // CP1251 translation) — the atlas was built for Unicode BMP already.
        // The old engine set TCMap.z = 1 + right - left (note the +1), used for both
        // the quad width and the advance. The quad texture spans [left..right].
        fontHeight = ini->r_float("mb_symbol_coords", "height");
        for (u32 cp = 0; cp <= 0xFFFF; ++cp)
        {
            xr_sprintf(keyBuf, sizeof(keyBuf), "%05d", cp);
            if (!ini->line_exist("mb_symbol_coords", keyBuf))
                continue;
            const Fvector3 v = ini->r_fvector3("mb_symbol_coords", keyBuf);
            // For mb format the key is already a codepoint; store it directly.
            // advance = 1 + (right - left), matching the old engine's TCMap.z.
            const float advance = 1.0f + std::max(0.0f, v.z - v.x);
            Glyph glyph{};
            glyph.TextureCoord = { static_cast<LONG>(v.x), static_cast<LONG>(v.y), static_cast<LONG>(v.z), static_cast<LONG>(v.y + fontHeight) };
            glyph.Abc.abcA = 0;
            glyph.Abc.abcB = static_cast<u32>(advance);
            glyph.Abc.abcC = 0;
            glyph.yOffset = 0;
            GlyphData[static_cast<int>(cp)] = glyph;
        }
    }
    else if (ini->section_exist("symbol_coords"))
    {
        // 256-entry format keyed by CP1251 byte. Row is "left, top, right" (r_fvector3).
        // The old engine set TCMap.z = right - left (width_correction was commented out:
        // `float d = 0.0f;`), and used that value for BOTH the quad width and the advance.
        // This is proportional spacing — narrow glyphs advance less, wide glyphs more.
        fontHeight = ini->r_float("symbol_coords", "height");
        for (u32 cp1251Byte = 0; cp1251Byte < 256; ++cp1251Byte)
        {
            xr_sprintf(keyBuf, sizeof(keyBuf), "%03d", cp1251Byte);
            if (!ini->line_exist("symbol_coords", keyBuf))
                continue;
            const Fvector3 v = ini->r_fvector3("symbol_coords", keyBuf);
            const LONG left = static_cast<LONG>(v.x);
            const LONG top = static_cast<LONG>(v.y);
            const LONG right = static_cast<LONG>(v.z);
            // advance = right - left (matches old engine exactly; no width_correction).
            addGlyph(static_cast<u8>(cp1251Byte), left, top, right,
                static_cast<LONG>(top + fontHeight), static_cast<float>(right - left));
        }
    }
    else if (ini->section_exist("char widths"))
    {
        // Per-character widths on a uniform grid (cpl = 16). Row is a single width
        // value which IS the advance (the old engine used it directly as TCMap.z).
        fontHeight = ini->r_float("char widths", "height");
        const int cpl = 16;
        for (u32 cp1251Byte = 0; cp1251Byte < 256; ++cp1251Byte)
        {
            xr_sprintf(keyBuf, sizeof(keyBuf), "%d", cp1251Byte);
            if (!ini->line_exist("char widths", keyBuf))
                continue;
            const float w = ini->r_float("char widths", keyBuf);
            const LONG left = static_cast<LONG>((cp1251Byte % cpl) * fontHeight);
            const LONG top = static_cast<LONG>((cp1251Byte / cpl) * fontHeight);
            addGlyph(static_cast<u8>(cp1251Byte), left, top, static_cast<LONG>(left + w),
                static_cast<LONG>(top + fontHeight), w);
        }
    }
    else if (ini->section_exist("font_size"))
    {
        // Monospace grid: every glyph has the same width/height. advance = width.
        fontHeight = ini->r_float("font_size", "height");
        const float width = ini->r_float("font_size", "width");
        const int cpl = ini->r_s32("font_size", "cpl");
        for (u32 cp1251Byte = 0; cp1251Byte < 256; ++cp1251Byte)
        {
            const LONG left = static_cast<LONG>((cp1251Byte % cpl) * width);
            const LONG top = static_cast<LONG>((cp1251Byte / cpl) * fontHeight);
            addGlyph(static_cast<u8>(cp1251Byte), left, top, static_cast<LONG>(left + width),
                static_cast<LONG>(top + fontHeight), width);
        }
    }
    else
    {
        // No recognized section — fall back to a monospace 8x8 grid so the font
        // is at least usable rather than empty.
        Msg("! r_font_legacy: no recognized metrics section in '%s', using 8x8 fallback", iniPath);
        fontHeight = 8.0f;
        const float width = 8.0f;
        const int cpl = 16;
        for (u32 cp1251Byte = 0; cp1251Byte < 256; ++cp1251Byte)
        {
            const LONG left = static_cast<LONG>((cp1251Byte % cpl) * width);
            const LONG top = static_cast<LONG>((cp1251Byte / cpl) * fontHeight);
            addGlyph(static_cast<u8>(cp1251Byte), left, top, static_cast<LONG>(left + width),
                static_cast<LONG>(top + fontHeight), width);
        }
    }

    CInifile::Destroy(ini);

    // 3. Match the resolution/DPI scaling applied to FreeType TTF fonts (Initialize2)
    //    so legacy bitmap fonts end up the same visual size at the same ui_text_scale.
    //    TTF bakes res_scale*ppi_scale into the rasterized atlas; for bitmap fonts we
    //    scale only the on-screen advance/quad width and line height. The atlas texture
    //    coordinates must stay in the original .dds pixel space, otherwise glyphs sample
    //    the wrong atlas cells and text appears garbled.
    const bool is_res_depend = !!READ_IF_EXISTS(pSettings, r_bool, Data.TextureName, "res_depend", TRUE);
    const bool is_dpi_depend = !!READ_IF_EXISTS(pSettings, r_bool, Data.TextureName, "dpi_depend", !is_res_depend);
    float res_scale = 1.0f;
    float ppi_scale = 1.0f;
    if (is_res_depend && Device.dwHeight > 0)
        res_scale = float(Device.dwHeight) / 900.0f;
#if defined(XR_PLATFORM_WINDOWS)
    if (is_dpi_depend)
    {
        HDC hDCScreen = GetDC(NULL);
        const float Hmm = (float)GetDeviceCaps(hDCScreen, VERTSIZE);
        const float Wmm = (float)GetDeviceCaps(hDCScreen, HORZSIZE);
        const float Hpx = (float)GetDeviceCaps(hDCScreen, VERTRES);
        const float Wpx = (float)GetDeviceCaps(hDCScreen, HORZRES);
        ReleaseDC(NULL, hDCScreen);
        int ppi = int(25.4f * sqrt(Hpx * Hpx + Wpx * Wpx) / sqrt(Hmm * Hmm + Wmm * Wmm));
        if (ppi > 0)
            ppi_scale = float(ppi) / 92.0f;
    }
#endif
    const float totalScale = res_scale * ppi_scale;
    if (totalScale != 1.0f)
    {
        // Bitmap fonts use a fixed-resolution .dds atlas. Scaling the texture
        // coordinates would sample the wrong atlas cells and produce garbled
        // glyphs. Only the on-screen advance/quad width and line height are
        // scaled so legacy fonts match the visual size of FreeType TTF fonts.
        for (auto& kv : GlyphData)
        {
            kv.second.Abc.abcA = static_cast<int>(kv.second.Abc.abcA * totalScale);
            kv.second.Abc.abcB = static_cast<u32>(kv.second.Abc.abcB * totalScale);
            kv.second.Abc.abcC = static_cast<int>(kv.second.Abc.abcC * totalScale);
        }
        fontHeight *= totalScale;
    }

    // 4. Load the .dds texture via the standard texturing path (the engine
    //    decompresses DXT5 itself — no manual decoding needed).
    pFontRender->Initialize(shader, textureName);

    // 5. Set the font height metric so CurrentHeight_() / line spacing work.
    fCurrentHeight = fontHeight;

    Msg("* Legacy font '%s' loaded: %u glyphs, height=%.1f, scale=%.2f",
        name, (u32)GlyphData.size(), fontHeight, totalScale);
}

void CGameFont::Initialize2(pcstr name, pcstr shader, pcstr style, u32 size)
{
    // Font renderer selection:
    //   - `r_font_legacy` == 1 forces the legacy X-Ray bitmap fonts (.dds + .ini).
    //   - otherwise FreeType TTF is used, but if no TTF/OTF files exist in
    //     $game_fonts$, we also fall back to legacy automatically.
    // The legacy path converts CP1251-keyed metrics into the UTF-8 GlyphData map
    // and reuses the same UTF-8 renderer (dxFontRender::OnRender) — the old
    // texture-based rendering code (ImprintChar/TCMap) is never restored.
    const bool forceLegacy = g_font_legacy;
    bool noTtf = false;
    if (!forceLegacy)
    {
        FS_FileSet ttfFiles;
        const size_t ttfCount = FS.file_list(ttfFiles, _game_fonts_, FS_ListFiles, "*.ttf");
        FS_FileSet otfFiles;
        const size_t otfCount = FS.file_list(otfFiles, _game_fonts_, FS_ListFiles, "*.otf");
        noTtf = (ttfCount == 0 && otfCount == 0);
    }

    if (forceLegacy || noTtf)
    {
        if (forceLegacy)
            Msg("~ r_font_legacy = 1, using legacy bitmap font for '%s'", name);
        else
            Msg("~ No TTF/OTF fonts in $game_fonts$, using legacy bitmap font for '%s'", name);
        InitializeLegacy(name, shader, style, size);
        return;
    }

    if (!bFreetypeInitialized)
    {
        InitializeFreetype();
        bFreetypeInitialized = true;
    }

    if (OurFont)
    {
        FT_Done_Face(OurFont);
        OurFont = nullptr;
    }
    if (m_fontReader)
    {
        FS.r_close(m_fontReader);
        m_fontReader = nullptr;
    }

    GlyphData.clear();

    ZeroMemory(&Style, sizeof(Style));
    Data.Size = static_cast<u16>(size);

    if (style != nullptr)
    {
        xr_string StyleDesc(style);
        xr_vector<xr_string> StyleTokens = split(StyleDesc, '|');
        for (const xr_string& token : StyleTokens)
        {
            if (token == "bold")
                Style.bold = 1;
            else if (token == "italic")
                Style.italic = 1;
            else if (token == "underline")
                Style.underline = 1;
            else if (token == "strike")
                Style.strike = 1;
        }
    }
    xr_vector<u32> FontBitmap;
    const u32 bitmapSize = TextureDimension * TextureDimension;
    FontBitmap.assign(bitmapSize, 0);

    int ppi = 96;
    float res_scale = 1.0f;
    float ppi_scale = 1.0f;

    // Font scaling flags are platform-independent; read them before the
    // platform-specific PPI block so resolution-based scaling works on POSIX too.
    const bool is_res_depend = !!READ_IF_EXISTS(pSettings, r_bool, Data.TextureName, "res_depend", TRUE);
    const bool is_dpi_depend = !!READ_IF_EXISTS(pSettings, r_bool, Data.TextureName, "dpi_depend", !is_res_depend);

    // Resolution-dependent scaling is the same on every platform: scale relative
    // to the reference 900px height. Without this, fonts stay tiny on 4K on POSIX.
    if (is_res_depend && Device.dwHeight > 0)
        res_scale = float(Device.dwHeight) / 900.0f;

#if defined(XR_PLATFORM_WINDOWS)
    HDC hDCScreen = GetDC(NULL);

    auto Hmm = (float)GetDeviceCaps(hDCScreen, VERTSIZE);
    auto Wmm = (float)GetDeviceCaps(hDCScreen, HORZSIZE);
    auto Hpx = (float)GetDeviceCaps(hDCScreen, VERTRES);
    auto Wpx = (float)GetDeviceCaps(hDCScreen, HORZRES);

    ReleaseDC(NULL, hDCScreen);

    ppi = int(25.4f * sqrt(Hpx * Hpx + Wpx * Wpx) / sqrt(Hmm * Hmm + Wmm * Wmm));
    if (ppi <= 0)
        ppi = 96;

    if (is_dpi_depend)
        ppi_scale = float(ppi) / 92.0f;
#endif

    auto fHeight = float(size) * res_scale * ppi_scale;
    if (fHeight < 1.0f)
        fHeight = 1.0f;

    pcstr fontPrefix = StringTable().GetCurrentFontPrefix().c_str();
    if (!fontPrefix || !fontPrefix[0])
    {
        fontPrefix = StringTable().GetCurrentLanguage().c_str();
        if (!fontPrefix || !fontPrefix[0])
        {
            Msg("! Font prefix/language is not set, falling back to 'eng'");
            fontPrefix = "eng";
        }
        else
        {
            Msg("! Font prefix is not set, using current language '%s'", fontPrefix);
        }
    }

    auto try_open_font = [&](pcstr prefix, pcstr fontName, string_path& outPath) -> IReader*
    {
        xr_string path = prefix;
        path += "\\";
        path += fontName;
        path += ".ttf";

        FS.update_path(outPath, _game_fonts_, path.c_str());
        return FS.r_open(outPath);
    };

    // Case-insensitive fallback for POSIX file systems. Font names in fonts.ltx
    // are lowercased before lookup (see ToLowerAscii above), but the actual TTF
    // files on disk may use mixed case (e.g. "Graffiti1CTT.ttf"). On case-
    // sensitive file systems the direct open then fails. Walk the font folder
    // once and pick the entry that matches the requested path case-insensitively.
    // FS_File::name is already stored lower-case, so the comparison is cheap.
    auto try_open_font_ci = [&](pcstr prefix, pcstr fontName, string_path& outPath) -> IReader*
    {
        if (IReader* direct = try_open_font(prefix, fontName, outPath))
            return direct;

        FS_FileSet files;
        if (FS.file_list(files, _game_fonts_, FS_ListFiles, "*.ttf") == 0)
            return nullptr;

        // file_list names are relative to $game_fonts$ and lower-case, e.g. "rus\arial.ttf".
        xr_string wanted = xr_string(prefix) + _DELIMITER + fontName + ".ttf";
        XRay::Utf8::ToLowerAscii(wanted.data());
        for (const FS_File& f : files)
        {
            if (f.name == wanted)
            {
                FS.update_path(outPath, _game_fonts_, f.name.c_str());
                if (IReader* r = FS.r_open(outPath))
                {
                    Msg("~ Font: case-insensitive match '%s'", f.name.c_str());
                    return r;
                }
            }
        }
        return nullptr;
    };

    string_path FullPath{};
    IReader* FontFile = try_open_font_ci(fontPrefix, name, FullPath);
    if (FontFile == nullptr && xr_stricmp(fontPrefix, "eng") != 0)
    {
        Msg("! Can't open font '%s' in prefix '%s', trying 'eng'", name, fontPrefix);
        FontFile = try_open_font_ci("eng", name, FullPath);
    }

    if (FontFile == nullptr)
    {
        Msg("! Can't open font file %s", name);

        FontFile = try_open_font_ci(fontPrefix, "default", FullPath);
        if (FontFile == nullptr && xr_stricmp(fontPrefix, "eng") != 0)
            FontFile = try_open_font_ci("eng", "default", FullPath);

        R_ASSERT3(FontFile != nullptr, "Can't find default font for prefix: %s", fontPrefix);
    }

    FT_Error FTError = FT_New_Memory_Face(FreetypeLib, (FT_Byte*)FontFile->pointer(), FontFile->length(), 0, &OurFont);
    R_ASSERT3(FTError == 0, "FT_New_Memory_Face returned error", FullPath);

    m_fontReader = FontFile;

    // Try to use the Unicode charmap. If the font only provides a legacy
    // Windows-1251 cmap (common for old STALKER TTFs), we will translate
    // character codes to Unicode codepoints when building the atlas.
    Data.HasUnicodeCharmap = (FT_Select_Charmap(OurFont, FT_ENCODING_UNICODE) == 0);
    if (!Data.HasUnicodeCharmap)
        Msg("! Font '%s' has no Unicode charmap, using CP1251 translation", name);

    u32 TargetX = 0;
    u32 TargetY = 0;
    u32 TargetX2 = 0;
    u32 TargetY2 = 0;

    FT_Size_RequestRec req;
    req.type = FT_SIZE_REQUEST_TYPE_CELL;
    req.width = 0;
    req.height = (uint32_t)(fHeight * 64.0f);
    req.horiResolution = 0;
    req.vertResolution = 0;
    FTError = FT_Request_Size(OurFont, &req);
    if (FTError != 0)
    {
        Msg("! FT_Request_Size failed for '%s' (height=%.2f), falling back to FT_Set_Pixel_Sizes", name, fHeight);
        FTError = FT_Set_Pixel_Sizes(OurFont, 0, size);
        R_ASSERT3(FTError == 0, "FT_Set_Pixel_Sizes returned error", FullPath);
    }

#define FT_CEIL(X) (((X + 63) & -64) / 64)

    float FontSizeInPixels = (float)(OurFont->size->metrics.ascender - OurFont->size->metrics.descender) / 64.0f;
    if (FontSizeInPixels < 1.0f)
        FontSizeInPixels = float(size);

    u32 CurrentRowHeight = (u32)FontSizeInPixels;

    auto CopyGlyphImageToAtlas = [this, &FontBitmap, &TargetX, &TargetX2, &TargetY, &TargetY2, &CurrentRowHeight, FontSizeInPixels](FT_Bitmap& GlyphBitmap)
    {
        const u32 glyphWidth = GlyphBitmap.width;
        const u32 glyphRows = GlyphBitmap.rows;

        if (glyphWidth > TextureDimension || glyphRows > TextureDimension)
        {
            Msg("! Glyph too large: %dx%d, skipping", glyphWidth, glyphRows);
            return;
        }

        TargetX2 = TargetX + glyphWidth;
        if (TargetX2 > TextureDimension)
        {
            TargetX = 0;
            TargetX2 = glyphWidth;
            TargetY += CurrentRowHeight + 5;
            CurrentRowHeight = (u32)FontSizeInPixels;
        }

        TargetY2 = TargetY + std::max((u32)FontSizeInPixels, glyphRows);
        CurrentRowHeight = std::max(CurrentRowHeight, TargetY2 - TargetY);
        R_ASSERT2(TargetY2 <= TextureDimension, "Font too large, or dimension texture is too small");

        const u32 TargetYSaved = TargetY;
        if (glyphRows <= (u32)FontSizeInPixels)
            TargetY = TargetY + (u32)(FontSizeInPixels - (float)glyphRows);

        const u32 yEnd = std::min<u32>(TargetY2, TargetY + glyphRows);

        const int pitch = GlyphBitmap.pitch;
        u8* srcRow = GlyphBitmap.buffer;
        if (pitch < 0 && glyphRows > 0)
            srcRow += pitch * (glyphRows - 1);

        switch (GlyphBitmap.pixel_mode)
        {
        case FT_PIXEL_MODE_GRAY:
        {
            for (u32 y = TargetY; y < yEnd; ++y, srcRow += pitch)
            {
                for (u32 x = TargetX; x < TargetX2; ++x)
                {
                    const u8 SourcePixel = srcRow[x - TargetX];

                    u32 FinalPixel = SourcePixel;
                    FinalPixel |= (SourcePixel << 8);
                    FinalPixel |= (SourcePixel << 16);
                    FinalPixel |= (SourcePixel << 24);

                    FontBitmap[(y * TextureDimension) + x] = FinalPixel;
                }
            }
        }
        break;
        case FT_PIXEL_MODE_MONO:
        {
            for (u32 y = TargetY; y < yEnd; ++y, srcRow += pitch)
            {
                for (u32 x = TargetX; x < TargetX2; ++x)
                {
                    const u32 sx = x - TargetX;
                    const u8 SourcePixel = (srcRow[sx >> 3] >> (7 - (sx & 7))) & 1 ? 0xFF : 0x00;

                    u32 FinalPixel = SourcePixel;
                    FinalPixel |= (SourcePixel << 8);
                    FinalPixel |= (SourcePixel << 16);
                    FinalPixel |= (SourcePixel << 24);

                    FontBitmap[(y * TextureDimension) + x] = FinalPixel;
                }
            }
        }
        break;
        case FT_PIXEL_MODE_BGRA:
        {
            for (u32 y = TargetY; y < yEnd; ++y, srcRow += pitch)
            {
                const u32* SourceImage = (const u32*)srcRow;
                for (u32 x = TargetX; x < TargetX2; ++x)
                {
                    const u32 SourcePixel = SourceImage[x - TargetX];

                    const u8 Alpha = (SourcePixel & 0x000000FF);
                    const u8 Red = (SourcePixel & 0x0000FF00) >> 8;
                    const u8 Green = (SourcePixel & 0x00FF0000) >> 16;
                    const u8 Blue = (SourcePixel & 0xFF000000) >> 24;

                    u32 FinalPixel = Alpha;
                    FinalPixel |= (u32(Blue) << 8);
                    FinalPixel |= (u32(Green) << 16);
                    FinalPixel |= (u32(Red) << 24);

                    FontBitmap[(y * TextureDimension) + x] = FinalPixel;
                }
            }
        }
        break;
        default:
            Msg("! Unsupported font bitmap pixel mode: %d", GlyphBitmap.pixel_mode);
            break;
        }

        TargetY = TargetYSaved;
    };

    u32 index = 0;

    auto LoadGlyph = [&](FT_UInt glyphIndex, FT_ULong charCode)
    {
        // We only support the Basic Multilingual Plane (BMP).
        if (charCode > 0xFFFF)
            return;

        // FreeType already gave us the glyph index for the current charmap.
        // Use it directly instead of FT_Get_Char_Index, which would interpret
        // the value against the active charmap again.
        int storageCode = static_cast<int>(charCode);
        if (!Data.HasUnicodeCharmap)
        {
            // Legacy Windows-1251 font: re-key the glyph under the Unicode
            // codepoint that corresponds to the CP1251 byte the font uses.
            // Bytes 0x00..0xFF map 1:1; any native code >0xFF has no reliable
            // CP1251 mapping (and cannot be addressed via UTF-8 text), so skip it
            // instead of storing it under a key the renderer would never look up.
            if (charCode <= 0xFF)
                storageCode = static_cast<int>(XRay::Utf8::CP1251ByteToCodepoint(static_cast<u8>(charCode)));
            else
            {
                Msg("! Font '%s': non-Unicode glyph with native code 0x%04X (>0xFF) "
                    "is not addressable via UTF-8, skipping", name, charCode);
                return;
            }
        }

        FTError = FT_Load_Glyph(OurFont, glyphIndex, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL);
        if (FTError != 0)
        {
            Msg("! FT_Load_Glyph failed for glyph %u (char 0x%04X) in font '%s': %d",
                glyphIndex, charCode, FullPath, FTError);
            return;
        }
        FT_GlyphSlot Glyph = OurFont->glyph;
        FT_Glyph_Metrics& GlyphMetrics = Glyph->metrics;

        CopyGlyphImageToAtlas(Glyph->bitmap);

        RECT region;
        region.left = TargetX;
        region.right = TargetX + Glyph->bitmap.width;
        region.top = TargetY;
        region.bottom = long(TargetY2);

        ABC widths;
        widths.abcA = FT_CEIL(GlyphMetrics.horiBearingX);
        widths.abcB = Glyph->bitmap.width;
        widths.abcC = FT_CEIL(GlyphMetrics.horiAdvance) - widths.abcB - widths.abcA;

        int GlyphTopScanlineOffset = int(FontSizeInPixels - Glyph->bitmap.rows);
        int yOffset = -Glyph->bitmap_top - GlyphTopScanlineOffset;
        yOffset += (int)FontSizeInPixels; // Return back to the center pos
        yOffset -= (int)(FontSizeInPixels / 4);

        GlyphData[storageCode] = { region, widths, yOffset };

        TargetX = TargetX2;
        TargetX += 4;
    };

    auto charCode = FT_Get_First_Char(OurFont, &index);
    while (index != 0)
    {
        LoadGlyph(index, charCode);
        charCode = FT_Get_Next_Char(OurFont, charCode, &index);
    }

    fCurrentHeight = FontSizeInPixels;

    // Account for the last row's actual height when sizing the atlas.
    TargetY2 = std::max(TargetY2, TargetY + CurrentRowHeight);

    string128 textureName;
    xr_sprintf(textureName, sizeof(textureName), "$user$%s", Data.TextureName ? Data.TextureName : Data.Name);

    auto TargetDemensionY = 16u;
    while (TargetDemensionY < TargetY2)
        TargetDemensionY *= 2u;

    Msg("* Font '%s' metrics: fHeight=%.2f FontSizeInPixels=%.2f ppi=%d res_scale=%.3f ppi_scale=%.3f atlas=%dx%d used=%d",
        Data.Name, fHeight, FontSizeInPixels, ppi, res_scale, ppi_scale, TextureDimension, TargetDemensionY, TargetY2);
    R_ASSERT2(TargetDemensionY <= TextureDimension, "Font too large, or dimension texture is too small");

    pFontRender->CreateFontAtlas(TextureDimension, TargetDemensionY, textureName, FontBitmap.data());

    pFontRender->Initialize(shader, textureName);
}

void CGameFont::OutSet(float x, float y)
{
    fCurrentX = x;
    fCurrentY = y;
}

void CGameFont::OutSetI(float x, float y) { OutSet(DI2PX(x), DI2PY(y)); }

void CGameFont::OnRender()
{
    ZoneScoped;
    ScopeLock g(&s_cs);
    if (!strings.empty())
    {
        pFontRender->OnRender(*this);
        strings.clear();
    }
}

float CGameFont::SizeOf_(int cChar)
{
    if (cChar < 0)
        cChar = u8(cChar);

    return WidthOf(cChar);
}

float CGameFont::SizeOf_(pcstr s)
{
    return WidthOf(s);
}

float CGameFont::CurrentHeight_() { return fCurrentHeight * vInterval.y * g_text_scale; }

void CGameFont::SetHeight(float S) { fCurrentHeight = S; }

void CGameFont::SetHeightI(float S)
{
    fCurrentHeight = S * Device.dwHeight;
}

const CGameFont::Glyph* CGameFont::GetGlyphInfo(int ch)
{
    auto symbolInfoIterator = GlyphData.find(ch);
    if (symbolInfoIterator == GlyphData.end())
        return nullptr;

    return &symbolInfoIterator->second;
}

float CGameFont::WidthOf(int ch)
{
    if (ch == '\t' || ch == '\n')
        return 0.f;

    if (const Glyph* glyphInfo = GetGlyphInfo(ch))
        // Apply the global text scale here (terminal width helper): WidthOf(pcstr)
        // sums per-codepoint widths through this method, so scaling once here
        // keeps both single-char and whole-string measurements consistent.
        return float(glyphInfo->Abc.abcA + glyphInfo->Abc.abcB + glyphInfo->Abc.abcC) * g_text_scale;

    return 0.f;
}

float CGameFont::WidthOf(pcstr str)
{
    if (!str || !str[0]) return 0;

    float size = 0;
    const float spacing = GetLetterSpacing();

    if (XRay::Utf8::IsValid(str))
    {
        for (const char* p = str; *p;)
        {
            size_t len = 0;
            const int cp = (int)XRay::Utf8::Decode(p, len);
            p += len ? len : 1;
            size += WidthOf(cp);
            size += spacing;
        }
        size -= spacing;
    }
    else
    {
        const int length = (int)xr_strlen(str);
        for (int i = 0; i < length; i++)
        {
            size += WidthOf((u8)str[i]);
            size += spacing;
        }
        size -= spacing;
    }

    return size;
}

u32 CGameFont::smart_strlen(pcstr S)
{
    if (!S || !S[0])
        return 0;

    // All TTF/OTF fonts are multibyte; count Unicode codepoints for UTF-8 input.
    if (IsMultibyte() && XRay::Utf8::IsValid(S))
        return static_cast<u32>(XRay::Utf8::LengthCodepoints(S));

    return static_cast<u32>(xr_strlen(S));
}

u16 CGameFont::GetCutLengthPos(float fTargetWidth, pcstr pszText)
{
    VERIFY(pszText);

    float fCurWidth = 0.0f;
    size_t lastPos = 0;

    // Fallback for non-UTF-8 strings: iterate bytes.
    if (!XRay::Utf8::IsValid(pszText))
    {
        for (pcstr p = pszText; *p; ++p)
        {
            const float fDelta = WidthOf(static_cast<u8>(*p));
            if ((fCurWidth + fDelta) > fTargetWidth)
                break;

            fCurWidth += fDelta;
            lastPos = static_cast<size_t>(p - pszText) + 1;
        }
        return static_cast<u16>(lastPos);
    }

    for (pcstr p = pszText; *p;)
    {
        size_t len = 0;
        const u32 cp = XRay::Utf8::Decode(p, len);
        const pcstr next = p + (len ? len : 1);

        float fDelta = WidthOf(static_cast<int>(cp));
        if (IsNeedSpaceCharacter(static_cast<xr_wide_char>(cp)))
            fDelta += WidthOf(' ');

        if ((fCurWidth + fDelta) > fTargetWidth)
            break;

        fCurWidth += fDelta;
        lastPos = static_cast<size_t>(next - pszText);
        p = next;
    }

    return static_cast<u16>(lastPos);
}

u16 CGameFont::SplitByWidth(u16* puBuffer, u16 uBufferSize, float fTargetWidth, pcstr pszText)
{
    VERIFY(puBuffer && uBufferSize && pszText);

    // Fallback for non-UTF-8 strings: iterate bytes.
    if (!XRay::Utf8::IsValid(pszText))
    {
        float fCurWidth = 0.0f;
        u16 nLines = 0;

        for (pcstr p = pszText; *p; ++p)
        {
            const float fDelta = WidthOf(static_cast<u8>(*p));

            if ((fCurWidth + fDelta) > fTargetWidth && nLines < uBufferSize)
            {
                fCurWidth = fDelta;
                puBuffer[nLines++] = static_cast<u16>(p - pszText);
            }
            else
                fCurWidth += fDelta;
        }
        return nLines;
    }

    float fCurWidth = 0.0f;
    u16 nLines = 0;
    pcstr p = pszText;
    u32 prevCp = 0;
    bool hasPrev = false;

    while (*p)
    {
        size_t len = 0;
        const u32 cp = XRay::Utf8::Decode(p, len);
        const pcstr next = p + (len ? len : 1);

        float fDelta = WidthOf(static_cast<int>(cp));
        if (IsNeedSpaceCharacter(static_cast<xr_wide_char>(cp)))
            fDelta += WidthOf(' ');

        const bool canBreak = hasPrev &&
            ((fCurWidth + fDelta) > fTargetWidth) &&
            (!IsBadStartCharacter(static_cast<xr_wide_char>(cp))) &&
            (*next != '\0') &&
            (!IsBadEndCharacter(static_cast<xr_wide_char>(prevCp)));

        if (canBreak && nLines < uBufferSize)
        {
            fCurWidth = fDelta;
            puBuffer[nLines++] = static_cast<u16>(p - pszText);
        }
        else
            fCurWidth += fDelta;

        prevCp = cp;
        hasPrev = true;
        p = next;
    }

    return nLines;
}

void CGameFont::MasterOut(bool bCheckDevice, bool bUseCoords, bool bScaleCoords, bool bUseSkip,
    float _x, float _y, float _skip, pcstr fmt, va_list p)
{
    if (bCheckDevice && (!Device.b_is_Active))
        return;

    String rs;

    rs.x = (bUseCoords ? (bScaleCoords ? (DI2PX(_x)) : _x) : fCurrentX);
    rs.y = (bUseCoords ? (bScaleCoords ? (DI2PY(_y)) : _y) : fCurrentY);
    rs.c = dwCurrentColor;
    rs.gradientColor = dwGradientColor;
    rs.height = fCurrentHeight * g_text_scale;
    rs.align = eCurrentAlignment;
    rs.gradient = fGradientEnabled;
    rs.gradientMode = fGradientMode;

    int vs_sz = vsnprintf(rs.string, sizeof(rs.string), fmt, p);
    rs.string[sizeof(rs.string) - 1] = 0;
    if (vs_sz == -1)
        return;

    // Expand GAME_ACTION_MARK sequences (0x1B + action_id) to key names
    // This was previously only done in the wide-char path (SizeOf_(xr_wide_char*)),
    // but the UTF-8 FreeType renderer needs it here.
    {
        char expanded[sizeof(rs.string)];
        char* dst = expanded;
        const char* src = rs.string;
        const char* const dst_end = expanded + sizeof(expanded) - 1;

        while (*src && dst < dst_end)
        {
            if (*src == GAME_ACTION_MARK)
            {
                ++src;
                if (!*src) break; // malformed: marker at end of string
                static_assert(kLASTACTION < type_max<u8>, "Modify the code to have more than 255 actions.");
                const EGameActions actionId = static_cast<EGameActions>(static_cast<u8>(*src));
                ++src;

                cpcstr binding = GetActionBinding(actionId);
                if (binding)
                {
                    for (const char* b = binding; *b && dst < dst_end; ++b)
                        *dst++ = *b;
                }
            }
            else
            {
                *dst++ = *src++;
            }
        }
        *dst = 0;
        xr_strcpy(rs.string, sizeof(rs.string), expanded);
    }

    if (!XRay::Utf8::IsValid(rs.string))
    {
        // If vsnprintf truncated a UTF-8 string, the last codepoint may be cut in half.
        // Try to recover a valid UTF-8 prefix by trimming up to 3 trailing bytes.
        // If that does not help, the source is likely CP1251 — convert it.
        const xr_string original(rs.string);
        size_t len = original.size();
        bool recovered = false;
        while (len > 0 && original.size() - len < 3)
        {
            --len;
            rs.string[len] = 0;
            if (XRay::Utf8::IsValid(rs.string))
            {
                recovered = true;
                break;
            }
        }

        if (!recovered)
        {
            // The source bytes are not valid UTF-8. Treat them as CP1251 (legacy
            // content or a caller that still passes narrow strings) and convert.
            // Log it: this path hides real bugs where callers feed CP1251 instead
            // of UTF-8, so make it visible instead of silently "fixing" the text.
            Msg("! CGameFont: non-UTF-8 text passed to Out*() (treated as CP1251): \"%s\"", original.c_str());
            strncpy_s(rs.string, sizeof(rs.string), original.c_str(), original.size());
            rs.string[sizeof(rs.string) - 1] = 0;
            rs.string_utf8 = XRay::Utf8::FromCP1251(rs.string);
        }
    }

    if (vs_sz)
    {
        ScopeLock g(&s_cs);
        strings.push_back(rs);
    }

    if (bUseSkip)
        OutSkip(_skip);
}

#define MASTER_OUT(CHECK_DEVICE, USE_COORDS, SCALE_COORDS, USE_SKIP, X, Y, SKIP, FMT) \
{ \
    va_list p; \
    va_start(p, fmt); \
    MasterOut(CHECK_DEVICE, USE_COORDS, SCALE_COORDS, USE_SKIP, X, Y, SKIP, FMT, p); \
    va_end(p); \
}

void __cdecl CGameFont::OutI(float _x, float _y, pcstr fmt, ...)
{
    MASTER_OUT(false, true, true, false, _x, _y, 0.0f, fmt);
}

void __cdecl CGameFont::Out(float _x, float _y, pcstr fmt, ...)
{
    MASTER_OUT(true, true, false, false, _x, _y, 0.0f, fmt);
}

void __cdecl CGameFont::OutNext(pcstr fmt, ...)
{
    MASTER_OUT(true, false, false, true, 0.0f, 0.0f, LineSpacing, fmt);
}

void CGameFont::OutNextVA(pcstr format, va_list args)
{
    MasterOut(true, false, false, true, 0.0f, 0.0f, LineSpacing, format, args);
}

void CGameFont::OutSkip(float val) { fCurrentY += val * CurrentHeight_(); }
