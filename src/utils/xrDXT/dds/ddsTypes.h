#pragma once
#include <windows.h>
#include <memory.h>

#if defined(WIN32_LEAN_AND_MEAN)
#include <mmsystem.h> // MAKEFOURCC
#endif

#include <dds/tPixel.h>
#include <dds/nvErrorCodes.h>

typedef	unsigned char	BYTE;
typedef	unsigned short	WORD;
typedef	unsigned long	DWORD;

typedef enum nvD3DFORMAT
{
    nvD3DFMT_UNKNOWN = 0,
    nvD3DFMT_R8G8B8 = 20,
    nvD3DFMT_A8R8G8B8 = 21,
    nvD3DFMT_X8R8G8B8 = 22,
    nvD3DFMT_R5G6B5 = 23,
    nvD3DFMT_X1R5G5B5 = 24,
    nvD3DFMT_A1R5G5B5 = 25,
    nvD3DFMT_A4R4G4B4 = 26,
    nvD3DFMT_R3G3B2 = 27,
    nvD3DFMT_A8 = 28,
    nvD3DFMT_A8R3G3B2 = 29,
    nvD3DFMT_X4R4G4B4 = 30,
    nvD3DFMT_A2B10G10R10 = 31,
    nvD3DFMT_A8B8G8R8 = 32,
    nvD3DFMT_X8B8G8R8 = 33,
    nvD3DFMT_G16R16 = 34,
    nvD3DFMT_A2R10G10B10 = 35,
    nvD3DFMT_A16B16G16R16 = 36,
    nvD3DFMT_A8P8 = 40,
    nvD3DFMT_P8 = 41,
    nvD3DFMT_L8 = 50,
    nvD3DFMT_A8L8 = 51,
    nvD3DFMT_A4L4 = 52,
    nvD3DFMT_V8U8 = 60,
    nvD3DFMT_L6V5U5 = 61,
    nvD3DFMT_X8L8V8U8 = 62,
    nvD3DFMT_Q8W8V8U8 = 63,
    nvD3DFMT_V16U16 = 64,
    nvD3DFMT_A2W10V10U10 = 67,
    nvD3DFMT_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y'),
    nvD3DFMT_R8G8_B8G8 = MAKEFOURCC('R', 'G', 'B', 'G'),
    nvD3DFMT_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2'),
    nvD3DFMT_G8R8_G8B8 = MAKEFOURCC('G', 'R', 'G', 'B'),
    nvD3DFMT_DXT1 = MAKEFOURCC('D', 'X', 'T', '1'),
    nvD3DFMT_DXT2 = MAKEFOURCC('D', 'X', 'T', '2'),
    nvD3DFMT_DXT3 = MAKEFOURCC('D', 'X', 'T', '3'),
    nvD3DFMT_DXT4 = MAKEFOURCC('D', 'X', 'T', '4'),
    nvD3DFMT_DXT5 = MAKEFOURCC('D', 'X', 'T', '5'),
    nvD3DFMT_3Dc = MAKEFOURCC('A', 'T', 'I', '2'),
    nvD3DFMT_D16_LOCKABLE = 70,
    nvD3DFMT_D32 = 71,
    nvD3DFMT_D15S1 = 73,
    nvD3DFMT_D24S8 = 75,
    nvD3DFMT_D24X8 = 77,
    nvD3DFMT_D24X4S4 = 79,
    nvD3DFMT_D16 = 80,
    nvD3DFMT_D32F_LOCKABLE = 82,
    nvD3DFMT_D24FS8 = 83,
    nvD3DFMT_L16 = 81,
    nvD3DFMT_VERTEXDATA = 100,
    nvD3DFMT_INDEX16 = 101,
    nvD3DFMT_INDEX32 = 102,
    nvD3DFMT_Q16W16V16U16 = 110,
    nvD3DFMT_MULTI2_ARGB8 = MAKEFOURCC('M', 'E', 'T', '1'),
    // Floating point surface formats
    // s10e5 formats (16-bits per channel)
    nvD3DFMT_R16F = 111,
    nvD3DFMT_G16R16F = 112,
    nvD3DFMT_A16B16G16R16F = 113,
    // IEEE s23e8 formats (32-bits per channel)
    nvD3DFMT_R32F = 114,
    nvD3DFMT_G32R32F = 115,
    nvD3DFMT_A32B32G32R32F = 116,
    nvD3DFMT_CxV8U8 = 117,
    nvD3DFMT_FORCE_DWORD = 0x7fffffff
} nvD3DFORMAT;

typedef enum nvRescaleTypes
{
    kRescaleNone, // no rescale
    kRescaleNearestPower2, // rescale to nearest power of two
    kRescaleBiggestPower2, // rescale to next bigger power of 2
    kRescaleSmallestPower2, // rescale to smaller power of 2 
    kRescaleNextSmallestPower2, // rescale to next smaller power of 2
    kRescalePreScale, // rescale to this size
    kRescaleRelScale, // relative rescale
} RescaleTypes;

typedef enum nvSharpenFilterTypes
{
    kSharpenFilterNone,
    kSharpenFilterNegative,
    kSharpenFilterLighter,
    kSharpenFilterDarker,
    kSharpenFilterContrastMore,
    kSharpenFilterContrastLess,
    kSharpenFilterSmoothen,
    kSharpenFilterSharpenSoft,
    kSharpenFilterSharpenMedium,
    kSharpenFilterSharpenStrong,
    kSharpenFilterFindEdges,
    kSharpenFilterContour,
    kSharpenFilterEdgeDetect,
    kSharpenFilterEdgeDetectSoft,
    kSharpenFilterEmboss,
    kSharpenFilterMeanRemoval,
    kSharpenFilterUnSharp,
    kSharpenFilterXSharpen,
    kSharpenFilterWarpSharp,
    kSharpenFilterCustom,
};

typedef enum nvMipMapGeneration
{
    kGenerateMipMaps = 30,
    kUseExistingMipMaps = 31,
    kNoMipMaps = 32,
    kCompleteMipMapChain = 33, // fill in missing MIP maps
};

typedef enum nvMipFilterTypes
{
    kMipFilterPoint,
    kMipFilterBox,
    kMipFilterTriangle,
    kMipFilterQuadratic,
    kMipFilterCubic,
    kMipFilterCatrom,
    kMipFilterMitchell,
    kMipFilterGaussian,
    kMipFilterSinc,
    kMipFilterBessel,
    kMipFilterHanning,
    kMipFilterHamming,
    kMipFilterBlackman,
    kMipFilterKaiser,
};

enum nvTextureFormats
{
    kDXT1,
    kDXT1a, // DXT1 with one bit alpha
    kDXT3, // explicit alpha
    kDXT5, // interpolated alpha
    k4444, // a4 r4 g4 b4
    k1555, // a1 r5 g5 b5
    k565, // a0 r5 g6 b5
    k8888, // a8 r8 g8 b8
    k888, // a0 r8 g8 b8
    k555, // a0 r5 g5 b5
    kP8c, // paletted color only
    kV8U8, // DuDv 
    kCxV8U8, // normal map
    kA8, // alpha only
    kP4c, // 16 bit color palette
    kQ8W8V8U8,
    kA8L8,
    kR32F,
    kA32B32G32R32F,
    kA16B16G16R16F,
    kL8, // luminance
    kP8a, // paletted with alpha
    kP4a, // 16 bit color palette with alpha
    kR16F, // single component fp16
    kDXT5_NM, // normal map compression. G = Y, A = X
    kX888, // aX r8 g8 b8
    kV16U16,
    kG16R16,
    kG16R16F,
    k3Dc,
    kL16,
    kUnknownTextureFormat = 0xFFFFFFFF,
};

enum nvTextureTypes
{
    kTextureTypeTexture2D,
    kTextureTypeCubeMap,
    kTextureTypeVolumeMap,
};

enum nvCompressionWeighting
{
    kLuminanceWeighting,
    kGreyScaleWeighting,
    kTangentSpaceNormalMapWeighting,
    kObjectSpaceNormalMapWeighting,
    kUserDefinedWeighting, // used values stored in 'weight'
};

enum nvNormalMapFilters
{
    kFilter4x = 1040,
    kFilter3x3 = 1041,
    kFilter5x5 = 1042,
    kFilter7x7 = 1043,
    kFilter9x9 = 1044,
    kFilterDuDv = 1045,
};

enum nvHeightConversionMethods
{
    kAlphaChannel = 1009,
    kAverageRGB = 1010,
    kBiasedRGB = 1011,
    kRed = 1012,
    kGreen = 1013,
    kBlue = 1014,
    kMaxRGB = 1015,
    kColorSpace = 1016,
    kNormalize = 1017,
    kNormalMapToHeightMap = 1018,
};

enum nvAlphaResult
{
    kAlphaUnchanged = 1033,
    kAlphaHeight = 1034,
    kAlphaSetToZero = 1035,
    kAlphaSetToOne = 1036,
};

enum nvQualitySetting
{
    kQualityFastest = 68,
    kQualityNormal = 69,
    kQualityProduction = 71, // typical value
    kQualityHighest = 72,
};

enum nvPixelFormat
{
    PF_RGBA,
    PF_FP32,
};

// filled in by reading a dds file
struct DDS_PIXELFORMAT
{
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwRGBAlphaBitMask;
};

class nvImageContainer
{
public:
    // loaded directly from the .dds header
    DDS_PIXELFORMAT m_ddpfPixelFormat;
    // in file read
    size_t width; // of MIP 0
    size_t height; // of MIP 0
    size_t depth; // for volume maps
    size_t actualMipMapCount;
    size_t nMIPMapsToLoad;
    bool bFoundAlphaInRead; // is alpha field present and non-zero
    // in the input file
    DWORD dwCubeMapFlags;
    size_t bits_per_component;
    size_t nPlanes; // number of planes in the file format
    bool bCompressed; // is file a compressed format
    size_t paletteSize; // 16 or 256 entries
    rgba_t palette[256];
    DWORD fmt; // D3DFORMAT specified in .dds file
    nvTextureFormats textureFormat;
    nvTextureTypes textureType;
    fpMipMappedImage fpMIPImage;
    fpMipMappedCubeMap fpMIPCubeMap;
    fpMipMappedVolumeMap fpMIPVolumeMap;
    RGBAMipMappedImage rgbaMIPImage;
    RGBAMipMappedCubeMap rgbaMIPCubeMap;
    RGBAMipMappedVolumeMap rgbaMIPVolumeMap;

    nvImageContainer()
    {
        bits_per_component = 0; // image's resolution in bits per pixel per plane
        paletteSize = 0;
        bFoundAlphaInRead = false;
        bCompressed = false;
        fmt = 0; // nvD3DFMT_UNKNOWN
        dwCubeMapFlags = 0;
        actualMipMapCount = 1;
        nMIPMapsToLoad = 1;
        depth = 0;
    }
};
