#include "stdafx.h"
#pragma hdrstop

#include "GameFont.h"
#include "xrCore/Text/StringConversion.hpp"
#ifndef _EDITOR
#include "Render.h"
#endif

extern ENGINE_API BOOL g_bRendering;
ENGINE_API Fvector2 g_current_font_scale = {1.0f, 1.0f};

#include "Include/xrRender/RenderFactory.h"
#include "Include/xrRender/FontRender.h"

CGameFont::CGameFont(pcstr section, u32 flags)
{
    pFontRender = GEnv.RenderFactory->CreateFontRender();
    fCurrentHeight = 0.0f;
    fXStep = 0.0f;
    fYStep = 0.0f;
    uFlags = flags;
    nNumChars = 0x100;
    TCMap = NULL;
    Initialize(pSettings->r_string(section, "shader"), pSettings->r_string(section, "texture"));
    if (pSettings->line_exist(section, "size"))
    {
        float sz = pSettings->r_float(section, "size");
        if (uFlags & fsDeviceIndependent)
            SetHeightI(sz);
        else
            SetHeight(sz);
    }
    if (pSettings->line_exist(section, "interval"))
        SetInterval(pSettings->r_fvector2(section, "interval"));
}

CGameFont::CGameFont(pcstr shader, pcstr texture, u32 flags)
{
    pFontRender = GEnv.RenderFactory->CreateFontRender();
    fCurrentHeight = 0.0f;
    fXStep = 0.0f;
    fYStep = 0.0f;
    uFlags = flags;
    nNumChars = 0x100;
    TCMap = NULL;
    Initialize(shader, texture);
}

void CGameFont::Initialize(pcstr cShader, pcstr cTextureName)
{
    string_path cTexture;

    LPCSTR _lang = pSettings->r_string("string_table", "font_prefix");
    bool is_di = strstr(cTextureName, "ui_font_hud_01") || strstr(cTextureName, "ui_font_hud_02") ||
        strstr(cTextureName, "ui_font_console_02");
    if (_lang && !is_di)
        strconcat(sizeof(cTexture), cTexture, cTextureName, _lang);
    else
        xr_strcpy(cTexture, sizeof(cTexture), cTextureName);

    uFlags &= ~fsValid;
    vTS.set(1.f, 1.f); // обязательно !!!

    eCurrentAlignment = alLeft;
    vInterval.set(1.f, 1.f);

    strings.reserve(128);

    // check ini exist
    string_path fn, buf;
    xr_strcpy(buf, cTexture);
    if (strext(buf))
        *strext(buf) = 0;
    R_ASSERT2(FS.exist(fn, "$game_textures$", buf, ".ini"), fn);
    CInifile* ini = CInifile::Create(fn);

    nNumChars = 0x100;
    TCMap = (Fvector*)xr_realloc((void*)TCMap, nNumChars * sizeof(Fvector));

    if (ini->section_exist("mb_symbol_coords"))
    {
        nNumChars = 0x10000;
        TCMap = (Fvector*)xr_realloc((void*)TCMap, nNumChars * sizeof(Fvector));
        uFlags |= fsMultibyte;
        fHeight = ini->r_float("mb_symbol_coords", "height");

        fXStep = ceil(fHeight / 2.0f);

        // Searching for the first valid character

        Fvector vFirstValid = {0, 0, 0};

        if (ini->line_exist("mb_symbol_coords", "09608"))
        {
            Fvector v = ini->r_fvector3("mb_symbol_coords", "09608");
            vFirstValid.set(v.x, v.y, 1 + v[2] - v[0]);
        }
        else
            for (u32 i = 0; i < nNumChars; i++)
            {
                xr_sprintf(buf, sizeof(buf), "%05d", i);
                if (ini->line_exist("mb_symbol_coords", buf))
                {
                    Fvector v = ini->r_fvector3("mb_symbol_coords", buf);
                    vFirstValid.set(v.x, v.y, 1 + v[2] - v[0]);
                    break;
                }
            }

        // Filling entire character table

        for (u32 i = 0; i < nNumChars; i++)
        {
            xr_sprintf(buf, sizeof(buf), "%05d", i);
            if (ini->line_exist("mb_symbol_coords", buf))
            {
                Fvector v = ini->r_fvector3("mb_symbol_coords", buf);
                TCMap[i].set(v.x, v.y, 1 + v[2] - v[0]);
            }
            else
                TCMap[i] = vFirstValid; // "unassigned" unprintable characters
        }

        // Special case for space
        TCMap[0x0020].set(0, 0, 0);
        // Special case for ideographic space
        TCMap[0x3000].set(0, 0, 0);
    }
    else if (ini->section_exist("symbol_coords"))
    {
        float d = 0.0f;
        //. if(ini->section_exist("width_correction"))
        //. d = ini->r_float("width_correction", "value");

        fHeight = ini->r_float("symbol_coords", "height");
        for (u32 i = 0; i < nNumChars; i++)
        {
            xr_sprintf(buf, sizeof(buf), "%03d", i);
            Fvector v = ini->r_fvector3("symbol_coords", buf);
            TCMap[i].set(v.x, v.y, v[2] - v[0] + d);
        }
    }
    else
    {
        if (ini->section_exist("char widths"))
        {
            fHeight = ini->r_float("char widths", "height");
            int cpl = 16;
            for (u32 i = 0; i < nNumChars; i++)
            {
                xr_sprintf(buf, sizeof(buf), "%d", i);
                float w = ini->r_float("char widths", buf);
                TCMap[i].set((i % cpl) * fHeight, (i / cpl) * fHeight, w);
            }
        }
        else
        {
            R_ASSERT(ini->section_exist("font_size"));
            fHeight = ini->r_float("font_size", "height");
            float width = ini->r_float("font_size", "width");
            const int cpl = ini->r_s32("font_size", "cpl");
            for (u32 i = 0; i < nNumChars; i++)
                TCMap[i].set((i % cpl) * width, (i / cpl) * fHeight, width);
        }
    }

    fCurrentHeight = fHeight;

    CInifile::Destroy(ini);

    // Shading
    pFontRender->Initialize(cShader, cTexture);
}

CGameFont::~CGameFont()
{
    if (TCMap)
        xr_free(TCMap);

    // Shading
    GEnv.RenderFactory->DestroyFontRender(pFontRender);
}

#define DI2PX(x) float(iFloor((x + 1) * float(GEnv.Render->getTarget()->get_width()) * 0.5f))
#define DI2PY(y) float(iFloor((y + 1) * float(GEnv.Render->getTarget()->get_height()) * 0.5f))

void CGameFont::OutSet(float x, float y)
{
    fCurrentX = x;
    fCurrentY = y;
}

void CGameFont::OutSetI(float x, float y) { OutSet(DI2PX(x), DI2PY(y)); }
u32 CGameFont::smart_strlen(pcstr S) { return (IsMultibyte() ? mbhMulti2Wide(NULL, NULL, 0, S) : xr_strlen(S)); }
void CGameFont::OnRender()
{
    pFontRender->OnRender(*this);
    strings.clear();
}

u16 CGameFont::GetCutLengthPos(float fTargetWidth, pcstr pszText)
{
    VERIFY(pszText);

    wchar_t wsStr[MAX_MB_CHARS], wsPos[MAX_MB_CHARS];
    float fCurWidth = 0.0f, fDelta = 0.0f;

    u16 len = mbhMulti2Wide(wsStr, wsPos, MAX_MB_CHARS, pszText);

    u16 i = 1;
    for (; i <= len; i++)
    {
        fDelta = GetCharTC(wsStr[i]).z - 2;

        if (IsNeedSpaceCharacter(wsStr[i]))
            fDelta += fXStep;

        if ((fCurWidth + fDelta) > fTargetWidth)
            break;
        else
            fCurWidth += fDelta;
    }

    return wsPos[i - 1];
}

u16 CGameFont::SplitByWidth(u16* puBuffer, u16 uBufferSize, float fTargetWidth, pcstr pszText)
{
    VERIFY(puBuffer && uBufferSize && pszText);

    wchar_t wsStr[MAX_MB_CHARS], wsPos[MAX_MB_CHARS];
    float fCurWidth = 0.0f, fDelta = 0.0f;
    u16 nLines = 0;

    u16 len = mbhMulti2Wide(wsStr, wsPos, MAX_MB_CHARS, pszText);

    for (u16 i = 1; i <= len; i++)
    {
        fDelta = GetCharTC(wsStr[i]).z - 2;

        if (IsNeedSpaceCharacter(wsStr[i]))
            fDelta += fXStep;

        if (((fCurWidth + fDelta) > fTargetWidth) && // overlength
            (!IsBadStartCharacter(wsStr[i])) && // can start with this character
            (i < len) && // is not the last character
            ((i > 1) && (!IsBadEndCharacter(wsStr[i - 1]))) // && // do not stop the string on a "bad" character
            // ( ( i > 1 ) && ( ! ( ( IsAlphaCharacter( wsStr[ i - 1 ] ) ) && ( IsAlphaCharacter( wsStr[ i ] ) ) ) ) )
            // // do
            // not split numbers or words
            )
        {
            fCurWidth = fDelta;
            VERIFY(nLines < uBufferSize);
            puBuffer[nLines++] = wsPos[i - 1];
        }
        else
            fCurWidth += fDelta;
    }

    return nLines;
}

void CGameFont::MasterOut(bool bCheckDevice, bool bUseCoords, bool bScaleCoords, bool bUseSkip, float _x, float _y,
    float _skip, pcstr fmt, va_list p)
{
    if (bCheckDevice && (!RDEVICE.b_is_Active))
        return;

    String rs;

    rs.x = (bUseCoords ? (bScaleCoords ? (DI2PX(_x)) : _x) : fCurrentX);
    rs.y = (bUseCoords ? (bScaleCoords ? (DI2PY(_y)) : _y) : fCurrentY);
    rs.c = dwCurrentColor;
    rs.height = fCurrentHeight;
    rs.align = eCurrentAlignment;
    int vs_sz = vsnprintf(rs.string, sizeof(rs.string), fmt, p);
    // VERIFY( ( vs_sz != -1 ) && ( rs.string[ vs_sz ] == '\0' ) );

    rs.string[sizeof(rs.string) - 1] = 0;
    if (vs_sz == -1)
    {
        return;
    }

    if (vs_sz)
        strings.push_back(rs);

    if (bUseSkip)
        OutSkip(_skip);
}

#define MASTER_OUT(CHECK_DEVICE, USE_COORDS, SCALE_COORDS, USE_SKIP, X, Y, SKIP, FMT)    \
    \
{                                                                                 \
        va_list p;                                                                       \
        va_start(p, fmt);                                                                \
        MasterOut(CHECK_DEVICE, USE_COORDS, SCALE_COORDS, USE_SKIP, X, Y, SKIP, FMT, p); \
        va_end(p);                                                                       \
    \
}

void __cdecl CGameFont::OutI(float _x, float _y, pcstr fmt, ...)
{
    MASTER_OUT(false, true, true, false, _x, _y, 0.0f, fmt);
};

void __cdecl CGameFont::Out(float _x, float _y, pcstr fmt, ...)
{
    MASTER_OUT(true, true, false, false, _x, _y, 0.0f, fmt);
};

void __cdecl CGameFont::OutNext(pcstr fmt, ...) { MASTER_OUT(TRUE, FALSE, FALSE, TRUE, 0.0f, 0.0f, 1.0f, fmt); };
void CGameFont::OutNextVA(pcstr format, va_list args)
{
    MasterOut(TRUE, FALSE, FALSE, TRUE, 0.0f, 0.0f, 1.0f, format, args);
}

void CGameFont::OutSkip(float val) { fCurrentY += val * CurrentHeight_(); }
float CGameFont::SizeOf_(const char cChar)
{
    return (GetCharTC((u16)(u8)(((IsMultibyte() && cChar == ' ')) ? 0 : cChar)).z * vInterval.x);
}

float CGameFont::SizeOf_(pcstr s)
{
    if (!(s && s[0]))
        return 0;

    if (IsMultibyte())
    {
        wchar_t wsStr[MAX_MB_CHARS];

        mbhMulti2Wide(wsStr, NULL, MAX_MB_CHARS, s);

        return SizeOf_(wsStr);
    }

    int len = xr_strlen(s);
    float X = 0;
    if (len)
        for (int j = 0; j < len; j++)
            X += GetCharTC((u16)(u8)s[j]).z;

    return (X * vInterval.x);
}

float CGameFont::SizeOf_(const wchar_t* wsStr)
{
    if (!(wsStr && wsStr[0]))
        return 0;

    unsigned int len = wsStr[0];
    float X = 0.0f, fDelta = 0.0f;

    if (len)
        for (unsigned int j = 1; j <= len; j++)
        {
            fDelta = GetCharTC(wsStr[j]).z - 2;
            if (IsNeedSpaceCharacter(wsStr[j]))
                fDelta += fXStep;
            X += fDelta;
        }

    return (X * vInterval.x);
}

float CGameFont::CurrentHeight_() { return fCurrentHeight * vInterval.y; }
void CGameFont::SetHeightI(float S)
{
    VERIFY(uFlags & fsDeviceIndependent);
    fCurrentHeight = S * RDEVICE.dwHeight;
};

void CGameFont::SetHeight(float S)
{
    VERIFY(uFlags & fsDeviceIndependent);
    fCurrentHeight = S;
};
