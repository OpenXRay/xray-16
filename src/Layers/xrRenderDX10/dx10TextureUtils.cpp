#include "stdafx.h"
#include "dx10TextureUtils.h"

namespace dx10TextureUtils
{
struct TextureFormatPairs
{
    D3DFORMAT m_dx9FMT;
    DXGI_FORMAT m_dx10FMT;
};

TextureFormatPairs TextureFormatList[] = {
    {D3DFMT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
    // D3DFMT_R8G8B8 Not available
    {D3DFMT_A8R8G8B8, DXGI_FORMAT_R8G8B8A8_UNORM}, // Not available
    // D3DFMT_X8R8G8B8 Not available
    //	TODO: DX10: Remove. Need only for nullrt
    //{ D3DFMT_R5G6B5,		DXGI_FORMAT_B5G6R5_UNORM },		// Not available
    {D3DFMT_R5G6B5, DXGI_FORMAT_R8G8B8A8_UNORM}, // Not available
    // D3DFMT_X1R5G5B5 Not available
    // D3DFMT_A1R5G5B5 Not available
    // D3DFMT_A4R4G4B4 Not available
    // D3DFMT_R3G3B2 Not available
    // D3DFMT_A8 DXGI_FORMAT_A8_UNORM
    // D3DFMT_A8R3G3B2 Not available
    // D3DFMT_X4R4G4B4 Not available
    // D3DFMT_A2B10G10R10 DXGI_FORMAT_R10G10B10A2
    {D3DFMT_A8B8G8R8, DXGI_FORMAT_R8G8B8A8_UNORM}, // & DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
    // D3DFMT_X8B8G8R8 Not available
    {D3DFMT_G16R16, DXGI_FORMAT_R16G16_UNORM},
    // D3DFMT_A2R10G10B10 Not available
    {D3DFMT_A16B16G16R16, DXGI_FORMAT_R16G16B16A16_UNORM},
    // D3DFMT_A8P8 Not available
    // D3DFMT_P8 Not available
    {D3DFMT_L8, DXGI_FORMAT_R8_UNORM}, // Note: Use .r swizzle in shader to duplicate red to other components to get
                                       // D3D9 behavior.
    // D3DFMT_A8L8 Not available
    // D3DFMT_A4L4 Not available
    {D3DFMT_V8U8, DXGI_FORMAT_R8G8_SNORM},
    // D3DFMT_L6V5U5 Not available
    // D3DFMT_X8L8V8U8 Not available
    {D3DFMT_Q8W8V8U8, DXGI_FORMAT_R8G8B8A8_SNORM}, {D3DFMT_V16U16, DXGI_FORMAT_R16G16_SNORM},
    // D3DFMT_W11V11U10 Not available
    // D3DFMT_A2W10V10U10 Not available
    // D3DFMT_UYVY Not available
    // D3DFMT_R8G8_B8G8 DXGI_FORMAT_G8R8_G8B8_UNORM (in DX9 the data was scaled up by 255.0f, but this can be handled in
    // shader code).
    // D3DFMT_YUY2 Not available
    // D3DFMT_G8R8_G8B8 DXGI_FORMAT_R8G8_B8G8_UNORM (in DX9 the data was scaled up by 255.0f, but this can be handled in
    // shader code).
    // D3DFMT_DXT1 DXGI_FORMAT_BC1_UNORM & DXGI_FORMAT_BC1_UNORM_SRGB
    // D3DFMT_DXT2 DXGI_FORMAT_BC1_UNORM & DXGI_FORMAT_BC1_UNORM_SRGB Note: DXT1 and DXT2 are the same from an
    // API/hardware
    // perspective... only difference was 'premultiplied alpha', which can be tracked by an application and doesn't need
    // a
    // separate format.
    // D3DFMT_DXT3 DXGI_FORMAT_BC2_UNORM & DXGI_FORMAT_BC2_UNORM_SRGB
    // D3DFMT_DXT4 DXGI_FORMAT_BC2_UNORM & DXGI_FORMAT_BC2_UNORM_SRGB Note: DXT3 and DXT4 are the same from an
    // API/hardware
    // perspective... only difference was 'premultiplied alpha', which can be tracked by an application and doesn't need
    // a
    // separate format.
    // D3DFMT_DXT5 DXGI_FORMAT_BC3_UNORM & DXGI_FORMAT_BC3_UNORM_SRGB
    // D3DFMT_D16 & D3DFMT_D16_LOCKABLE DXGI_FORMAT_D16_UNORM
    // D3DFMT_D32 Not available
    // D3DFMT_D15S1 Not available
    // D3DFMT_D24S8 Not available
    {D3DFMT_D24S8, DXGI_FORMAT_D24_UNORM_S8_UINT},
    {D3DFMT_D24X8, DXGI_FORMAT_R24G8_TYPELESS}, // DXGI_FORMAT_D24_UNORM_S8_UINT},	// Not available
    // D3DFMT_D24X4S4 Not available
    { D3DFMT_D16_LOCKABLE, DXGI_FORMAT_D16_UNORM },
    { D3DFMT_D32F_LOCKABLE, DXGI_FORMAT_R32_TYPELESS },
    { D3DFMT_D32F_LOCKABLE, DXGI_FORMAT_D32_FLOAT },
    { D3DFMT_D32S8X24, DXGI_FORMAT_D32_FLOAT_S8X24_UINT},
    // D3DFMT_D24FS8 Not available
    // D3DFMT_S1D15 Not available
    // D3DFMT_S8D24 DXGI_FORMAT_D24_UNORM_S8_UINT
    // D3DFMT_X8D24 Not available
    // D3DFMT_X4S4D24 Not available
    // D3DFMT_L16 DXGI_FORMAT_R16_UNORM Note: Use .r swizzle in shader to duplicate red to other components to get D3D9
    // behavior.
    // D3DFMT_INDEX16 DXGI_FORMAT_R16_UINT
    // D3DFMT_INDEX32 DXGI_FORMAT_R32_UINT
    // D3DFMT_Q16W16V16U16 DXGI_FORMAT_R16G16B16A16_SNORM
    // D3DFMT_MULTI2_ARGB8 Not available
    // D3DFMT_R16F DXGI_FORMAT_R16_FLOAT
    {D3DFMT_G16R16F, DXGI_FORMAT_R16G16_FLOAT}, {D3DFMT_A16B16G16R16F, DXGI_FORMAT_R16G16B16A16_FLOAT},
    {D3DFMT_R32F, DXGI_FORMAT_R32_FLOAT}, {D3DFMT_R16F, DXGI_FORMAT_R16_FLOAT},
    //{ D3DFMT_G32R32F,		DXGI_FORMAT_R32G32_FLOAT},
    {D3DFMT_A32B32G32R32F, DXGI_FORMAT_R32G32B32A32_FLOAT},
    // D3DFMT_CxV8U8 Not available
    // D3DDECLTYPE_FLOAT1 DXGI_FORMAT_R32_FLOAT
    // D3DDECLTYPE_FLOAT2 DXGI_FORMAT_R32G32_FLOAT
    // D3DDECLTYPE_FLOAT3 DXGI_FORMAT_R32G32B32_FLOAT
    // D3DDECLTYPE_FLOAT4 DXGI_FORMAT_R32G32B32A32_FLOAT
    // D3DDECLTYPED3DCOLOR Not available
    // D3DDECLTYPE_UBYTE4 DXGI_FORMAT_R8G8B8A8_UINT Note: Shader gets UINT values, but if Direct3D 9 style integral
    // floats
    // are needed (0.0f, 1.0f... 255.f), UINT can just be converted to float32 in shader.
    // D3DDECLTYPE_SHORT2 DXGI_FORMAT_R16G16_SINT Note: Shader gets SINT values, but if Direct3D 9 style integral floats
    // are
    // needed, SINT can just be converted to float32 in shader.
    // D3DDECLTYPE_SHORT4 DXGI_FORMAT_R16G16B16A16_SINT Note: Shader gets SINT values, but if Direct3D 9 style integral
    // floats are needed, SINT can just be converted to float32 in shader.
    // D3DDECLTYPE_UBYTE4N DXGI_FORMAT_R8G8B8A8_UNORM
    // D3DDECLTYPE_SHORT2N DXGI_FORMAT_R16G16_SNORM
    // D3DDECLTYPE_SHORT4N DXGI_FORMAT_R16G16B16A16_SNORM
    // D3DDECLTYPE_USHORT2N DXGI_FORMAT_R16G16_UNORM
    // D3DDECLTYPE_USHORT4N DXGI_FORMAT_R16G16B16A16_UNORM
    // D3DDECLTYPE_UDEC3 Not available
    // D3DDECLTYPE_DEC3N Not available
    // D3DDECLTYPE_FLOAT16_2 DXGI_FORMAT_R16G16_FLOAT
    // D3DDECLTYPE_FLOAT16_4 DXGI_FORMAT_R16G16B16A16_FLOAT
};

DXGI_FORMAT ConvertTextureFormat(D3DFORMAT dx9FMT)
{
    int arrayLength = sizeof(TextureFormatList) / sizeof(TextureFormatList[0]);
    for (int i = 0; i < arrayLength; ++i)
    {
        if (TextureFormatList[i].m_dx9FMT == dx9FMT)
            return TextureFormatList[i].m_dx10FMT;
    }

    VERIFY(!"ConvertTextureFormat didn't find appropriate dx10 texture format!");
    return DXGI_FORMAT_UNKNOWN;
}

D3DFORMAT ConvertTextureFormat(DXGI_FORMAT dx10FMT)
{
    int arrayLength = sizeof(TextureFormatList) / sizeof(TextureFormatList[0]);
    for (int i = 0; i < arrayLength; ++i)
    {
        if (TextureFormatList[i].m_dx10FMT == dx10FMT)
            return TextureFormatList[i].m_dx9FMT;
    }

    VERIFY(!"ConvertTextureFormat didn't find appropriate dx9 texture format!");
    return D3DFMT_UNKNOWN;
}
}
