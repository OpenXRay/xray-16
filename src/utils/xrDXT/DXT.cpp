#include "stdafx.h"
#pragma warning(push)
#pragma warning(disable:4244)
#pragma warning(disable:4018)
#include "dxtlib.h"
#include <nvtt/nvtt.h>
#pragma warning(pop)
#include "ETextureParams.h"
#include "dds.h"

BOOL APIENTRY DllMain(HANDLE hModule, u32 ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

static HFILE gFileOut;
static HFILE gFileIn;

const u32 fcc_DXT1 = MAKEFOURCC('D', 'X', 'T', '1');
const u32 fcc_DXT2 = MAKEFOURCC('D', 'X', 'T', '2');
const u32 fcc_DXT3 = MAKEFOURCC('D', 'X', 'T', '3');
const u32 fcc_DXT4 = MAKEFOURCC('D', 'X', 'T', '4');
const u32 fcc_DXT5 = MAKEFOURCC('D', 'X', 'T', '5');

void __cdecl WriteDTXnFile(DWORD count, void* buffer, void* userData)
{
    if (count == sizeof(DDS_HEADER))
    {
        DDS_HEADER* hdr = (DDS_HEADER*)buffer;
        if (hdr->dwSize == count)
        {
            switch (hdr->ddspf.dwFourCC)
            {
            case fcc_DXT1:
            case fcc_DXT2:
            case fcc_DXT3:
            case fcc_DXT4:
            case fcc_DXT5:
                hdr->ddspf.dwRGBBitCount = 0;
                break;
            }
        }
    }
    _write(gFileOut, buffer, count);
}

class DDSWriter : public nvtt::OutputHandler
{
public:
    HFILE& file;

    DDSWriter(HFILE& file);
    virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) override;
    virtual bool writeData(const void* data, int size) override;
};

DDSWriter::DDSWriter(HFILE& file) : file(file)
{
}

void DDSWriter::beginImage(int size, int width, int height, int depth, int face, int miplevel)
{
}

bool DDSWriter::writeData(const void* data, int size)
{
    if (size == sizeof(DDS_HEADER))
    {
        DDS_HEADER* hdr = (DDS_HEADER*)data;
        if (hdr->dwSize == size)
        {
            switch (hdr->ddspf.dwFourCC)
            {
            case fcc_DXT1:
            case fcc_DXT2:
            case fcc_DXT3:
            case fcc_DXT4:
            case fcc_DXT5:
                hdr->ddspf.dwRGBBitCount = 0;
                break;
            }
        }
    }
    _write(file, data, size);
    return true;
}

class DDSErrorHandler : public nvtt::ErrorHandler
{
public:
    virtual void error(nvtt::Error e) override;
};

void DDSErrorHandler::error(nvtt::Error e)
{
    const char* msg;
    switch (e)
    {
    default:
    case nvtt::Error_Unknown:               msg = "Unknown error"; break;
    case nvtt::Error_InvalidInput:          msg = "Invalid input"; break;
    case nvtt::Error_UnsupportedFeature:    msg = "Unsupported feature"; break;
    case nvtt::Error_CudaError:             msg = "CUDA error"; break;
    case nvtt::Error_FileOpen:              msg = "File open error"; break;
    case nvtt::Error_FileWrite:             msg = "File write error"; break;
    }
    MessageBox(0, msg, "DXT compress error", MB_ICONERROR | MB_OK);
}

void __cdecl ReadDTXnFile(DWORD count, void *buffer, void* userData)
{
    _read(gFileIn, buffer, count);
}

HRESULT WriteCompressedData(void* data, int miplevel, u32 size)
{
    _write(gFileOut, data, size);
    FillMemory(data, size, 0xff);
    return 0;
}

ENGINE_API u32* Build32MipLevel(u32 &_w, u32 &_h, u32 &_p, u32 *pdwPixelSrc, STextureParams* fmt, float blend)
{
    R_ASSERT(pdwPixelSrc);
    R_ASSERT(_w % 2 == 0);
    R_ASSERT(_h % 2 == 0);
    R_ASSERT(_p % 4 == 0);
    u32 dwDestPitch = (_w/2)*4;
    u32* pNewData = xr_alloc<u32>((_h/2)*dwDestPitch);
    u8* pDest = (u8*)pNewData;
    u8* pSrc = (u8*)pdwPixelSrc;
    float mixed_a = (float)u8(fmt->fade_color >> 24);
    float mixed_r = (float)u8(fmt->fade_color >> 16);
    float mixed_g = (float)u8(fmt->fade_color >> 8);
    float mixed_b = (float)u8(fmt->fade_color >> 0);
    float inv_blend = 1.f - blend;
    for (u32 y = 0; y < _h; y += 2)
    {
        u8* pScanline = pSrc + y*_p;
        for (u32 x = 0; x < _w; x += 2)
        {
            u8*	p1 = pScanline + x * 4;
            u8*	p2 = p1 + 4;    if (1 == _w) p2 = p1;
            u8*	p3 = p1 + _p;   if (1 == _h) p3 = p1;
            u8*	p4 = p2 + _p;   if (1 == _h) p4 = p2;
            float c_r = float(u32(p1[0]) + u32(p2[0]) + u32(p3[0]) + u32(p4[0])) / 4.f;
            float c_g = float(u32(p1[1]) + u32(p2[1]) + u32(p3[1]) + u32(p4[1])) / 4.f;
            float c_b = float(u32(p1[2]) + u32(p2[2]) + u32(p3[2]) + u32(p4[2])) / 4.f;
            float c_a = float(u32(p1[3]) + u32(p2[3]) + u32(p3[3]) + u32(p4[3])) / 4.f;
            if (fmt->flags.is(STextureParams::flFadeToColor))
            {
                c_r = c_r*inv_blend + mixed_r*blend;
                c_g = c_g*inv_blend + mixed_g*blend;
                c_b = c_b*inv_blend + mixed_b*blend;
            }
            if (fmt->flags.is(STextureParams::flFadeToAlpha))
            {
                c_a = c_a*inv_blend + mixed_a*blend;
            }
            float A = c_a + c_a / 8.f;
            int _r = int(c_r);	clamp(_r, 0, 255);	*pDest++ = u8(_r);
            int _g = int(c_g);	clamp(_g, 0, 255);	*pDest++ = u8(_g);
            int _b = int(c_b);	clamp(_b, 0, 255);	*pDest++ = u8(_b);
            int _a = int(A);	clamp(_a, 0, 255);	*pDest++ = u8(_a);
        }
    }
    _w /= 2; _h /= 2; _p = _w * 4;
    return pNewData;
}

IC u32 GetPowerOf2Plus1(u32 v)
{
    u32 cnt = 0;
    while (v)
    {
        v >>= 1;
        cnt++;
    }
    return cnt;
}

void FillRect(u8* data, u8* new_data, u32 offs, u32 pitch, u32 h, u32 full_pitch)
{
    for (u32 i = 0; i < h; i++)
    {
        CopyMemory(data + (full_pitch*i + offs), new_data + i*pitch, pitch);
    }
}

int DXTCompressImage(LPCSTR out_name, u8* raw_data, u32 w, u32 h, u32 pitch,
    STextureParams* fmt, u32 depth)
{
    R_ASSERT(0 != w && 0 != h);
    gFileOut = _open(out_name, _O_WRONLY | _O_BINARY | _O_CREAT | _O_TRUNC, _S_IWRITE);
    if (gFileOut == -1)
    {
        fprintf(stderr, "Can't open output file %s\n", out_name);
        return 0;
    }
    bool result = false;
    nvtt::InputOptions inOpt;
    auto layout = fmt->type == STextureParams::ttCubeMap ? nvtt::TextureType_Cube : nvtt::TextureType_2D;
    inOpt.setTextureLayout(layout, w, h);
    if (fmt->flags.is(STextureParams::flGenerateMipMaps))	inOpt.setMipmapGeneration(true);
    else													inOpt.setMipmapGeneration(false);
    inOpt.setWrapMode(nvtt::WrapMode_Clamp);
    inOpt.setNormalMap(false);
    inOpt.setConvertToNormalMap(false);
    inOpt.setGamma(2.2f, 2.2f);
    inOpt.setNormalizeMipmaps(false);
    nvtt::CompressionOptions compOpt;
    compOpt.setQuality(nvtt::Quality_Highest);
    compOpt.setQuantization(fmt->flags.is(STextureParams::flDitherColor), false,
        fmt->flags.is(STextureParams::flBinaryAlpha));
    switch (fmt->fmt)
    {
    case STextureParams::tfDXT1: 	compOpt.setFormat(nvtt::Format_DXT1); 	break;
    case STextureParams::tfADXT1: 	compOpt.setFormat(nvtt::Format_DXT1a); 	break;
    case STextureParams::tfDXT3: 	compOpt.setFormat(nvtt::Format_DXT3); 	break;
    case STextureParams::tfDXT5: 	compOpt.setFormat(nvtt::Format_DXT5); 	break;
    case STextureParams::tfRGB: 	compOpt.setFormat(nvtt::Format_RGB); 	break;
    case STextureParams::tfRGBA: 	compOpt.setFormat(nvtt::Format_RGBA); 	break;
    }
    switch (fmt->mip_filter)
    {
    case STextureParams::kMIPFilterAdvanced:    break;
    case STextureParams::kMIPFilterBox:         inOpt.setMipmapFilter(nvtt::MipmapFilter_Box);      break;
    case STextureParams::kMIPFilterTriangle:    inOpt.setMipmapFilter(nvtt::MipmapFilter_Triangle); break;
    case STextureParams::kMIPFilterKaiser:      inOpt.setMipmapFilter(nvtt::MipmapFilter_Kaiser);   break;
    }
    nvtt::OutputOptions outOpt;
    DDSWriter writer(gFileOut);
    outOpt.setOutputHandler(&writer);
    DDSErrorHandler handler;
    outOpt.setErrorHandler(&handler);
    if (fmt->flags.is(STextureParams::flGenerateMipMaps) && STextureParams::kMIPFilterAdvanced == fmt->mip_filter)
    {
        inOpt.setMipmapGeneration(false);
        u8* pImagePixels = 0;
        int numMipmaps = GetPowerOf2Plus1(__min(w, h));
        u32 line_pitch = w*2*4;
        pImagePixels = xr_alloc<u8>(line_pitch*h);
        u32 w_offs = 0;
        u32 dwW = w;
        u32 dwH = h;
        u32 dwP = pitch;
        u32* pLastMip = xr_alloc<u32>(w*h*4);
        CopyMemory(pLastMip, raw_data, w*h*4);
        FillRect(pImagePixels, (u8*)pLastMip, w_offs, pitch, dwH, line_pitch);
        w_offs += dwP;
        float inv_fade = clampr(1.f - float(fmt->fade_amount) / 100.f, 0.f, 1.f);
        float blend = fmt->flags.is_any(STextureParams::flFadeToColor | STextureParams::flFadeToAlpha)
            ? inv_fade : 1.f;
        for (int i = 1; i < numMipmaps; i++)
        {
            u32* pNewMip = Build32MipLevel(dwW, dwH, dwP, pLastMip, fmt, i < fmt->fade_delay ? 0.f : 1.f - blend);
            FillRect(pImagePixels, (u8*)pNewMip, w_offs, dwP, dwH, line_pitch);
            xr_free(pLastMip);
            pLastMip = pNewMip;
            pNewMip = 0;
            w_offs += dwP;
        }
        xr_free(pLastMip);
        RGBAImage pImage(w*2, h);
        rgba_t* pixels = pImage.pixels();
        u8* pixel = pImagePixels;
        for (u32 k = 0; k < w*2*h; k++, pixel += 4)
        {
            pixels[k].set(pixel[2], pixel[1], pixel[0], pixel[3]);
        }
        inOpt.setMipmapData(pixels, w, h);
        result = nvtt::Compressor().process(inOpt, compOpt, outOpt);
        xr_free(pImagePixels);
    }
    else
    {
        RGBAImage pImage(w, h);
        rgba_t* pixels = pImage.pixels();
        u8* pixel = raw_data;
        for (u32 k = 0; k<w*h; k++, pixel += 4)
            pixels[k].set(pixel[2], pixel[1], pixel[0], pixel[3]);
        inOpt.setMipmapData(pixels, w, h);
        result = nvtt::Compressor().process(inOpt, compOpt, outOpt);
    }
    _close(gFileOut);
    if (!result)
    {
        unlink(out_name);
        return 0;
    }
    return 1;
}

extern int DXTCompressBump(LPCSTR out_name, u8* raw_data, u8* normal_map,
    u32 w, u32 h, u32 pitch, STextureParams* fmt, u32 depth);

extern "C" __declspec(dllexport)
int  __stdcall DXTCompress(LPCSTR out_name, u8* raw_data, u8* normal_map,
    u32 w, u32 h, u32 pitch, STextureParams* fmt, u32 depth)
{
    switch (fmt->type)
    {
    case STextureParams::ttImage:
    case STextureParams::ttCubeMap:
    case STextureParams::ttNormalMap:
    case STextureParams::ttTerrain:
        return DXTCompressImage(out_name, raw_data, w, h, pitch, fmt, depth);
        break;
    case STextureParams::ttBumpMap:
        return DXTCompressBump(out_name, raw_data, normal_map, w, h, pitch, fmt, depth);
        break;
    default: NODEFAULT;
    }
    return -1;
}
