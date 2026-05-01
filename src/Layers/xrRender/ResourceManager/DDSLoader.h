#pragma once

#include "TextureManager.h"
#include <nvrhi/nvrhi.h>

// DDS File Format Loader
// Week 1 - Day 2: Task 2.1

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  DDS FILE FORMAT CONSTANTS
// ═══════════════════════════════════════════════════

#define DDS_MAGIC 0x20534444  // "DDS " in little endian

// DDSD flags (dwFlags in DDS_HEADER)
#define DDSD_CAPS         0x00000001
#define DDSD_HEIGHT       0x00000002
#define DDSD_WIDTH        0x00000004
#define DDSD_PITCH        0x00000008
#define DDSD_PIXELFORMAT  0x00001000
#define DDSD_MIPMAPCOUNT  0x00020000
#define DDSD_LINEARSIZE   0x00080000
#define DDSD_DEPTH        0x00800000

// DDPF flags (dwFlags in DDS_PIXELFORMAT)
#define DDPF_ALPHAPIXELS  0x00000001
#define DDPF_ALPHA        0x00000002
#define DDPF_FOURCC       0x00000004
#define DDPF_RGB          0x00000040
#define DDPF_YUV          0x00000200
#define DDPF_LUMINANCE    0x00020000

// DDSCAPS flags (dwCaps in DDS_CAPS2)
#define DDSCAPS_COMPLEX   0x00000008
#define DDSCAPS_TEXTURE   0x00001000
#define DDSCAPS_MIPMAP    0x00400000

// DDSCAPS2 flags
#define DDSCAPS2_CUBEMAP            0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000
#define DDSCAPS2_VOLUME             0x00200000

// Common FourCC codes
#define FOURCC_DXT1 0x31545844  // "DXT1"
#define FOURCC_DXT2 0x32545844  // "DXT2"
#define FOURCC_DXT3 0x33545844  // "DXT3"
#define FOURCC_DXT4 0x34545844  // "DXT4"
#define FOURCC_DXT5 0x35545844  // "DXT5"
#define FOURCC_DX10 0x30315844  // "DX10"
#define FOURCC_BC4U 0x55344342  // "BC4U"
#define FOURCC_BC4S 0x53344342  // "BC4S"
#define FOURCC_BC5U 0x55354342  // "BC5U"
#define FOURCC_BC5S 0x53354342  // "BC5S"
#define FOURCC_ATI1 0x31495441  // "ATI1" (BC4)
#define FOURCC_ATI2 0x32495441  // "ATI2" (BC5)

// ═══════════════════════════════════════════════════
//  DDS FILE STRUCTURES
// ═══════════════════════════════════════════════════

#pragma pack(push, 1)

struct DDS_PIXELFORMAT {
    u32 dwSize;           // Must be 32
    u32 dwFlags;          // Pixel format flags (DDPF_*)
    u32 dwFourCC;         // FourCC code (DXT1, DXT5, etc.)
    u32 dwRGBBitCount;    // Bits per pixel (16, 24, 32)
    u32 dwRBitMask;       // Red channel mask
    u32 dwGBitMask;       // Green channel mask
    u32 dwBBitMask;       // Blue channel mask
    u32 dwABitMask;       // Alpha channel mask
};

struct DDS_HEADER {
    u32 dwSize;              // Must be 124
    u32 dwFlags;             // Surface description flags (DDSD_*)
    u32 dwHeight;            // Height in pixels
    u32 dwWidth;             // Width in pixels
    u32 dwPitchOrLinearSize; // Pitch for uncompressed, total size for compressed
    u32 dwDepth;             // Depth for volume textures
    u32 dwMipMapCount;       // Number of mipmap levels
    u32 dwReserved1[11];     // Reserved
    DDS_PIXELFORMAT ddspf;   // Pixel format
    u32 dwCaps;              // Surface caps (DDSCAPS_*)
    u32 dwCaps2;             // Additional caps (DDSCAPS2_*)
    u32 dwCaps3;             // Reserved
    u32 dwCaps4;             // Reserved
    u32 dwReserved2;         // Reserved
};

// DX10 extended header (for BC6/BC7 and other modern formats)
enum DDSDxgiFormat : u32 {
    DDSDxgiFormat_UNKNOWN = 0,
    DDSDxgiFormat_R32_FLOAT = 41,
    DDSDxgiFormat_BC1_UNORM = 71,
    DDSDxgiFormat_BC1_UNORM_SRGB = 72,
    DDSDxgiFormat_BC2_UNORM = 74,
    DDSDxgiFormat_BC2_UNORM_SRGB = 75,
    DDSDxgiFormat_BC3_UNORM = 77,
    DDSDxgiFormat_BC3_UNORM_SRGB = 78,
    DDSDxgiFormat_BC4_UNORM = 80,
    DDSDxgiFormat_BC4_SNORM = 81,
    DDSDxgiFormat_BC5_UNORM = 83,
    DDSDxgiFormat_BC5_SNORM = 84,
    DDSDxgiFormat_BC6H_UF16 = 95,
    DDSDxgiFormat_BC6H_SF16 = 96,
    DDSDxgiFormat_BC7_UNORM = 98,
    DDSDxgiFormat_BC7_UNORM_SRGB = 99,
};

enum D3D10_RESOURCE_DIMENSION : u32 {
    D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
    D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
    D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4,
};

struct DDS_HEADER_DX10 {
    DDSDxgiFormat dxgiFormat;
    D3D10_RESOURCE_DIMENSION resourceDimension;
    u32 miscFlag;        // D3D10_RESOURCE_MISC_GENERATE_MIPS, etc.
    u32 arraySize;       // For texture arrays
    u32 miscFlags2;      // Alpha mode, etc.
};

#pragma pack(pop)

// ═══════════════════════════════════════════════════
//  PARSED DDS DATA
// ═══════════════════════════════════════════════════

struct DDSMipLevel {
    const u8* data;      // Pointer into file data
    u32 size;            // Bytes for this mip level
    u32 width;           // Mip width
    u32 height;          // Mip height
    u32 depth;           // Mip depth (for 3D textures)
    u32 rowPitch;        // Bytes per row (for GPU upload)
    u32 slicePitch;      // Bytes per slice (for 3D textures / GPU upload)
};

struct DDSData {
    // Texture description
    TextureDesc desc;

    // Mipmap data (all mips for all array slices)
    // For cubemaps: [+X, -X, +Y, -Y, +Z, -Z] with their mips
    // For arrays: [slice0_mip0, slice0_mip1, ..., slice1_mip0, slice1_mip1, ...]
    xr_vector<DDSMipLevel> mipLevels;

    // Owned copy of file data (so mipLevel pointers remain valid)
    xr_vector<u8> fileDataCopy;

    // Source file path (for debugging)
    shared_str filePath;

    // Total data size
    u64 totalDataSize = 0;

    // Validation
    bool isValid = false;
    shared_str errorMessage;

    // ═══════════════════════════════════════════════════
    //  DYNAMIC/VIDEO TEXTURE SUPPORT
    // ═══════════════════════════════════════════════════

    enum class TextureType {
        Static,      // Normal DDS texture - loaded once
        Video,       // Video texture (.ogm/.avi) - needs per-frame decode
        Sequence     // Animated sequence (.seq) - frame-based playback
    };

    TextureType type = TextureType::Static;

    // Video texture state (only valid if type == Video)
    struct VideoState {
        CTheoraSurface* theoraSurface = nullptr;  // Theora decoder (for .ogm)
        xr_vector<u32> frameBuffer;               // Decoded frame RGBA data
        u32 frameWidth = 0;                       // Actual video width
        u32 frameHeight = 0;                      // Actual video height
        u32 textureWidth = 0;                     // Pow2-padded texture width (for pitch)
        u32 textureHeight = 0;                    // Pow2-padded texture height (for pitch)
        bool needsUpdate = false;                 // Frame has changed, need writeTexture
    };

    VideoState* videoState = nullptr;  // Only allocated for video textures

    // Sequence texture state (only valid if type == Sequence)
    struct SequenceState {
        xr_vector<DDSMipLevel> frameData;     // Metadata for each frame (width, height, pitch)
        xr_vector<xr_vector<u8>> framePixels; // Actual pixel data for each frame (owned)
        xr_vector<u32> frameBuffer;           // Current frame data (uploaded to GPU)
        u32 frameWidth = 0;                   // Frame width
        u32 frameHeight = 0;                  // Frame height
        u32 textureWidth = 0;                 // Texture width (may be padded)
        u32 textureHeight = 0;                // Texture height (may be padded)
        u32 fps = 0;                          // Frames per second
        u32 msPerFrame = 0;                   // Milliseconds per frame (1000 / fps)
        bool cycled = false;                  // Ping-pong animation (forward then reverse)
        bool needsUpdate = false;             // Frame has changed, need writeTexture
        u32 currentFrame = 0;                 // Current frame index
        u32 lastUpdateTime = 0;               // Absolute time of last frame change (ms)
    };

    SequenceState* sequenceState = nullptr;  // Only allocated for sequence textures

    // Destructor must be in .cpp to avoid incomplete type issues with CTheoraSurface
    ~DDSData();

    // Move constructor and move assignment (for proper ownership transfer)
    DDSData(DDSData&& other) noexcept;
    DDSData& operator=(DDSData&& other) noexcept;

    // Delete copy constructor and copy assignment (non-copyable due to raw pointers)
    DDSData(const DDSData&) = delete;
    DDSData& operator=(const DDSData&) = delete;

    // Default constructor
    DDSData() = default;
};

// ═══════════════════════════════════════════════════
//  DDS LOADER CLASS
// ═══════════════════════════════════════════════════

class DDSLoader {
public:
    DDSLoader() = default;
    ~DDSLoader() = default;

    // ═══════════════════════════════════════════════════
    //  LOADING METHODS
    // ═══════════════════════════════════════════════════

    // Load texture from file on disk
    // Automatically detects file type and loads:
    // - .dds → Static DDS texture
    // - .ogm → Theora video texture (dynamic, per-frame decode)
    // - .avi → AVI video texture (not yet implemented)
    static bool LoadFromFile(const char* filePath, DDSData& outData);

    // Load DDS from memory buffer
    static bool LoadFromMemory(
        const u8* data,
        size_t dataSize,
        DDSData& outData,
        const char* debugName = nullptr
    );

    // Load specific mip range (optimization for streaming)
    // Loads only the specified mip levels instead of the entire file
    static bool LoadMipRange(
        const char* filePath,
        u32 startMip,
        u32 mipCount,
        DDSData& outData
    );

    // ═══════════════════════════════════════════════════
    //  VIDEO TEXTURE LOADING
    // ═══════════════════════════════════════════════════

    // Load video texture (.ogm format)
    // Creates a dynamic texture that needs per-frame updates
    static bool LoadVideoTexture(const char* filePath, DDSData& outData);

    // Load sequence texture (.seq format)
    // Pre-loads all frame textures and sets up frame buffer for updates
    static bool LoadSequenceTexture(const char* filePath, DDSData& outData);

    // Update video texture for current frame
    // Decodes the next frame and marks frameBuffer for GPU upload
    // Returns true if frame changed and needs writeTexture()
    static bool UpdateVideoFrame(DDSData& data, u32 currentTime);

    // Update sequence texture for current frame
    // Copies frame pixels to frameBuffer and marks for GPU upload
    // Returns true if frame changed and needs writeTexture()
    static bool UpdateSequenceFrame(DDSData& data, u32 currentFrame);

    // ═══════════════════════════════════════════════════
    //  FORMAT CONVERSION
    // ═══════════════════════════════════════════════════

    // Convert DDS FourCC to NVRHI format
    static nvrhi::Format GetFormatFromFourCC(u32 fourCC);

    // Convert DXGI format to NVRHI format
    static nvrhi::Format GetFormatFromDXGI(DDSDxgiFormat dxgiFormat);

    // ═══════════════════════════════════════════════════
    //  HELPER METHODS
    // ═══════════════════════════════════════════════════

    // Calculate size of a single mip level
    static u32 CalculateMipSize(
        u32 width,
        u32 height,
        u32 depth,
        nvrhi::Format format
    );

    // Calculate mip dimensions
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
    // ═══════════════════════════════════════════════════
    //  INTERNAL PARSING
    // ═══════════════════════════════════════════════════

    static bool ParseHeader(
        const u8* data,
        size_t dataSize,
        DDS_HEADER& outHeader,
        DDS_HEADER_DX10* outHeaderDX10,  // May be null
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

} // namespace xray::render::resources
