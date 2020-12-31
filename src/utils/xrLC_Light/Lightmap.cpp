// Lightmap.cpp: implementation of the CLightmap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Lightmap.h"
#include "xrDeflector.h"
#include "xrDXTC.h"
#include "xrImage_Filter.h"
#include "xrface.h"
#include "serialize.h"
#include "ETextureParams.h"

extern "C" XR_IMPORT
bool __stdcall DXTCompress(pcstr out_name, u8* raw_data, u8* normal_map,
    u32 w, u32 h, u32 pitch, STextureParams* fmt, u32 depth);

// extern BOOL ApplyBorders	(lm_layer &lm, u32 ref);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLightmap::CLightmap() {}
CLightmap::~CLightmap() {}
CLightmap* CLightmap::read_create() { return xr_new<CLightmap>(); }
void CLightmap::Capture(CDeflector* D, int b_u, int b_v, int s_u, int s_v, BOOL bRotated)
{
    // Allocate 512x512 texture if needed
    if (lm.surface.empty())
        lm.create(c_LMAP_size, c_LMAP_size);

    // Addressing
    xr_vector<UVtri> tris;
    D->RemapUV(
        tris, b_u + BORDER, b_v + BORDER, s_u - 2 * BORDER, s_v - 2 * BORDER, c_LMAP_size, c_LMAP_size, bRotated);

    // Capture faces and setup their coords
    for (UVIt T = tris.begin(); T != tris.end(); ++T)
    {
        UVtri& P = *T;
        Face* F = P.owner;
        F->lmap_layer = this;
        F->AddChannel(P.uv[0], P.uv[1], P.uv[2]);
    }

    // Perform BLIT
    lm_layer& L = D->layer;
    if (!bRotated)
    {
        u32 real_H = (L.height + 2 * BORDER);
        u32 real_W = (L.width + 2 * BORDER);
        blit(lm, c_LMAP_size, c_LMAP_size, L, real_W, real_H, b_u, b_v, 254 - BORDER);
    }
    else
    {
        u32 real_H = (L.height + 2 * BORDER);
        u32 real_W = (L.width + 2 * BORDER);
        blit_r(lm, c_LMAP_size, c_LMAP_size, L, real_W, real_H, b_u, b_v, 254 - BORDER);
    }
}

//////////////////////////////////////////////////////////////////////
IC u32 convert(float a)
{
    if (a <= 0)
        return 0;
    else if (a >= 1)
        return 255;
    else
        return iFloor(a * 255.f);
}
IC void pixel(int x, int y, b_texture* T, u32 C = color_rgba(0, 255, 0, 0))
{
    if (x < 0)
        return;
    else if (x >= (int)T->dwWidth)
        return;
    if (y < 0)
        return;
    else if (y >= (int)T->dwHeight)
        return;
    T->pSurface[y * T->dwWidth + x] = C;
}
IC void line(int x1, int y1, int x2, int y2, b_texture* T)
{
    int dx = _abs(x2 - x1);
    int dy = _abs(y2 - y1);
    int sx = x2 >= x1 ? 1 : -1;
    int sy = y2 >= y1 ? 1 : -1;

    if (dy <= dx)
    {
        int d = (dy << 1) - dx;
        int d1 = dy << 1;
        int d2 = (dy - dx) << 1;

        pixel(x1, y1, T);

        for (int x = x1 + sx, y = y1, i = 1; i <= dx; i++, x += sx)
        {
            if (d > 0)
            {
                d += d2;
                y += sy;
            }
            else
                d += d1;
            pixel(x, y, T);
        }
    }
    else
    {
        int d = (dx << 1) - dy;
        int d1 = dx << 1;
        int d2 = (dx - dy) << 1;

        pixel(x1, y1, T);
        for (int x = x1, y = y1 + sy, i = 1; i <= dy; i++, y += sy)
        {
            if (d > 0)
            {
                d += d2;
                x += sx;
            }
            else
                d += d1;
            pixel(x, y, T);
        }
    }
}

void CLightmap::Save(LPCSTR path)
{
    static int lmapNameID = 0;
    ++lmapNameID;

    Logger.Phase("Saving...");

    // Borders correction
    Logger.Status("Borders...");
    for (u32 _y = 0; _y < c_LMAP_size; _y++)
    {
        for (u32 _x = 0; _x < c_LMAP_size; _x++)
        {
            u32 offset = _y * c_LMAP_size + _x;
            if (lm.marker[offset] >= (254 - BORDER))
                lm.marker[offset] = 255;
            else
                lm.marker[offset] = 0;
        }
    }
    for (u32 ref = 254; ref > (254 - 16); ref--)
    {
        ApplyBorders(lm, ref);
        Logger.Progress(1.f - float(ref) / float(254 - 16));
    }
    Logger.Progress(1.f);

    xr_vector<u32> lm_packed;
    lm.Pack(lm_packed);
    xr_vector<u32> hemi_packed;
    lm.Pack_hemi(hemi_packed);

    lm_texture.bHasAlpha = TRUE;
    lm_texture.dwWidth = lm.width;
    lm_texture.dwHeight = lm.height;
    lm_texture.pSurface = NULL;

    lm.destroy();

    // Saving			(DXT5.dds)
    Logger.Status("Compression base...");
    {
        string_path FN;
        xr_sprintf(lm_texture.name, "lmap#%d", lmapNameID);
        xr_sprintf(FN, "%s%s_1.dds", path, lm_texture.name);
        u8* raw_data = (u8*)(&*lm_packed.begin());
        u32 w = lm_texture.dwWidth; // lm.width;
        u32 h = lm_texture.dwHeight; // lm.height;
        u32 pitch = w * 4;

        STextureParams fmt;
        fmt.fmt = STextureParams::tfDXT5;
        fmt.flags.set(STextureParams::flDitherColor, FALSE);
        fmt.flags.set(STextureParams::flGenerateMipMaps, FALSE);
        fmt.flags.set(STextureParams::flBinaryAlpha, FALSE);
        DXTCompress(FN, raw_data, 0, w, h, pitch, &fmt, 4);
    }
    lm_packed.clear();
    Logger.Status("Compression hemi..."); //.
    {
        u32 w = lm_texture.dwWidth; // lm.width;
        u32 h = lm_texture.dwHeight; // lm.height;
        u32 pitch = w * 4;

        string_path FN;
        xr_sprintf(lm_texture.name, "lmap#%d", lmapNameID);
        xr_sprintf(FN, "%s%s_2.dds", path, lm_texture.name);
        u8* raw_data = (u8*)(&*hemi_packed.begin());

        STextureParams fmt;
        fmt.fmt = STextureParams::tfDXT5;
        fmt.flags.set(STextureParams::flDitherColor, FALSE);
        fmt.flags.set(STextureParams::flGenerateMipMaps, FALSE);
        fmt.flags.set(STextureParams::flBinaryAlpha, FALSE);
        DXTCompress(FN, raw_data, 0, w, h, pitch, &fmt, 4);
    }
}
/*
    lm_layer					lm;
    b_texture					lm_texture;
*/
void CLightmap::read(INetReader& r)
{
    lm.read(r);
    ::read(r, lm_texture);
}
void CLightmap::write(IWriter& w) const
{
    lm.write(w);
    ::write(w, lm_texture);
}
