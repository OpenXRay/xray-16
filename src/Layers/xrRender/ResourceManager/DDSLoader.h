#pragma once

#include "TextureManager.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::resources {

#define DDS_MAGIC 0x20534444

#define DDSD_CAPS         0x00000001
#define DDSD_HEIGHT       0x00000002
#define DDSD_WIDTH        0x00000004
#define DDSD_PITCH        0x00000008
#define DDSD_PIXELFORMAT  0x00001000
#define DDSD_MIPMAPCOUNT  0x00020000
#define DDSD_LINEARSIZE   0x00080000
#define DDSD_DEPTH        0x00800000

#define DDPF_ALPHAPIXELS  0x00000001
#define DDPF_ALPHA        0x00000002
#define DDPF_FOURCC       0x00000004
#define DDPF_RGB          0x00000040
#define DDPF_YUV          0x00000200
#define DDPF_LUMINANCE    0x00020000

#define DDSCAPS_COMPLEX   0x00000008
#define DDSCAPS_TEXTURE   0x00001000
#define DDSCAPS_MIPMAP    0x00400000

#define DDSCAPS2_CUBEMAP            0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000
#define DDSCAPS2_VOLUME             0x00200000

#define FOURCC_DXT1 0x31545844
#define FOURCC_DXT2 0x32545844
#define FOURCC_DXT3 0x33545844
#define FOURCC_DXT4 0x34545844
#define FOURCC_DXT5 0x35545844
#define FOURCC_DX10 0x30315844
#define FOURCC_BC4U 0x55344342
#define FOURCC_BC4S 0x53344342
#define FOURCC_BC5U 0x55354342
#define FOURCC_BC5S 0x53354342
#define FOURCC_ATI1 0x31495441
#define FOURCC_ATI2 0x32495441

#pragma pack(push, 1)

struct DDS_PIXELFORMAT {
    u32 dwSize;
    u32 dwFlags;
    u32 dwFourCC;
    u32 dwRGBBitCount;
    u32 dwRBitMask;
    u32 dwGBitMask;
    u32 dwBBitMask;
    u32 dwABitMask;
};

struct DDS_HEADER {
    u32 dwSize;
    u32 dwFlags;
    u32 dwHeight;
    u32 dwWidth;
    u32 dwPitchOrLinearSize;
    u32 dwDepth;
    u32 dwMipMapCount;
    u32 dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    u32 dwCaps;
    u32 dwCaps2;
    u32 dwCaps3;
    u32 dwCaps4;
    u32 dwReserved2;
};

enum DXGI_FORMAT : u32 {
    DXGI_FORMAT_UNKNOWN = 0,
    DXGI_FORMAT_BC1_UNORM = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB = 72,
    DXGI_FORMAT_BC2_UNORM = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB = 75,
    DXGI_FORMAT_BC3_UNORM = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB = 78,
    DXGI_FORMAT_BC4_UNORM = 80,
    DXGI_FORMAT_BC4_SNORM = 81,
    DXGI_FORMAT_BC5_UNORM = 83,
    DXGI_FORMAT_BC5_SNORM = 84,
    DXGI_FORMAT_BC6H_UF16 = 95,
    DXGI_FORMAT_BC6H_SF16 = 96,
    DXGI_FORMAT_BC7_UNORM = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB = 99,
};

enum D3D10_RESOURCE_DIMENSION : u32 {
    D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
    D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
    D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4,
};

struct DDS_HEADER_DX10 {
    DXGI_FORMAT dxgiFormat;
    D3D10_RESOURCE_DIMENSION resourceDimension;
    u32 miscFlag;
    u32 arraySize;
    u32 miscFlags2;
};

#pragma pack(pop)

struct DDSMipLevel {
    const u8* data;
    u32 size;
    u32 width;
    u32 height;
    u32 depth;
    u32 rowPitch;
    u32 slicePitch;
};

struct DDSData {
    TextureDesc desc;

    xr_vector<DDSMipLevel> mipLevels;

    xr_vector<u8> fileDataCopy;

    shared_str filePath;

    u64 totalDataSize = 0;

    bool isValid = false;
    shared_str errorMessage;

    enum class TextureType {
        Static,
        Video,
        Sequence
    };

    TextureType type = TextureType::Static;

    struct VideoState {
        CTheoraSurface* theoraSurface = nullptr;
        xr_vector<u32> frameBuffer;
        u32 frameWidth = 0;
        u32 frameHeight = 0;
        u32 textureWidth = 0;
        u32 textureHeight = 0;
        bool needsUpdate = false;
    };

    VideoState* videoState = nullptr;

    struct SequenceState {
        xr_vector<DDSMipLevel> frameData;
        xr_vector<u32> frameBuffer;
        u32 frameWidth = 0;
        u32 frameHeight = 0;
        u32 textureWidth = 0;
        u32 textureHeight = 0;
        u32 fps = 0;
        u32 msPerFrame = 0;
        bool cycled = false;
        bool needsUpdate = false;
    };

    SequenceState* sequenceState = nullptr;

    ~DDSData();

    DDSData(DDSData&& other) noexcept;
    DDSData& operator=(DDSData&& other) noexcept;

    DDSData(const DDSData&) = delete;
    DDSData& operator=(const DDSData&) = delete;

    DDSData() = default;
};

class DDSLoader {
public:
    DDSLoader() = default;
    ~DDSLoader() = default;

    static bool LoadFromFile(const char* filePath, DDSData& outData);

    static bool LoadFromMemory(
        const u8* data,
        size_t dataSize,
        DDSData& outData,
        const char* debugName = nullptr
    );

    static bool LoadMipRange(
        const char* filePath,
        u32 startMip,
        u32 mipCount,
        DDSData& outData
    );

    static bool LoadVideoTexture(const char* filePath, DDSData& outData);

    static bool LoadSequenceTexture(const char* filePath, DDSData& outData);

    static bool UpdateVideoFrame(DDSData& data, u32 currentTime);

    static bool UpdateSequenceFrame(DDSData& data, u32 currentFrame);

    static nvrhi::Format GetFormatFromFourCC(u32 fourCC);

    static nvrhi::Format GetFormatFromDXGI(DXGI_FORMAT dxgiFormat);

    static u32 CalculateMipSize(
        u32 width,
        u32 height,
        u32 depth,
        nvrhi::Format format
    );

    static void CalculateMipDimensions(
        u32 baseWidth,
        u32 baseHeight,
        u32 baseDepth,
        u32 mipLevel,
        u32& outWidth,
        u32& outHeight,
        u32& outDepth
    );

private:

    static bool ParseHeader(
        const u8* data,
        size_t dataSize,
        DDS_HEADER& outHeader,
        DDS_HEADER_DX10* outHeaderDX10,
        const u8*& outTextureData,
        size_t& outTextureDataSize
    );

    static bool ParseMipLevels(
        const u8* textureData,
        size_t textureDataSize,
        const TextureDesc& desc,
        xr_vector<DDSMipLevel>& outMipLevels
    );

    static bool ValidateHeader(const DDS_HEADER& header);
};

}
