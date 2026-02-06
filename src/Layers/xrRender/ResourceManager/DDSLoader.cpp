#include "stdafx.h"
#include "DDSLoader.h"
#include "xrCore/FS.h"
#include "xrEngine/xrTheora_Surface.h"

// DDS File Format Loader Implementation
// Week 1 - Day 2: Task 2.1
// Week 6: Added video texture support (.ogm/.avi)

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  DDSDATA DESTRUCTOR
// ═══════════════════════════════════════════════════

DDSData::~DDSData() {
    // Clean up video state
    if (videoState) {
        if (videoState->theoraSurface) {
            xr_delete(videoState->theoraSurface);
        }
        xr_delete(videoState);
    }

    // Clean up sequence state
    if (sequenceState) {
        xr_delete(sequenceState);
    }
}

// Move constructor - transfer ownership
DDSData::DDSData(DDSData&& other) noexcept
    : desc(std::move(other.desc))
    , mipLevels(std::move(other.mipLevels))
    , fileDataCopy(std::move(other.fileDataCopy))
    , filePath(std::move(other.filePath))
    , totalDataSize(other.totalDataSize)
    , isValid(other.isValid)
    , errorMessage(std::move(other.errorMessage))
    , type(other.type)
    , videoState(other.videoState)      // Transfer pointer ownership
    , sequenceState(other.sequenceState) // Transfer pointer ownership
{
    // CRITICAL: Null out the moved-from pointers to prevent double-delete
    other.videoState = nullptr;
    other.sequenceState = nullptr;
}

// Move assignment operator - transfer ownership
DDSData& DDSData::operator=(DDSData&& other) noexcept {
    if (this != &other) {
        // Clean up our current state first
        if (videoState) {
            if (videoState->theoraSurface) {
                xr_delete(videoState->theoraSurface);
            }
            xr_delete(videoState);
        }
        if (sequenceState) {
            xr_delete(sequenceState);
        }

        // Transfer from other
        desc = std::move(other.desc);
        mipLevels = std::move(other.mipLevels);
        fileDataCopy = std::move(other.fileDataCopy);
        filePath = std::move(other.filePath);
        totalDataSize = other.totalDataSize;
        isValid = other.isValid;
        errorMessage = std::move(other.errorMessage);
        type = other.type;
        videoState = other.videoState;
        sequenceState = other.sequenceState;

        // Debug logging for sequence state transfer
        if (sequenceState) {
            Msg("* [DDSData move assignment] Transferred sequenceState: fps=%u, msPerFrame=%u, frames=%u",
                sequenceState->fps, sequenceState->msPerFrame, (u32)sequenceState->frameData.size());
        }

        // CRITICAL: Null out the moved-from pointers to prevent double-delete
        other.videoState = nullptr;
        other.sequenceState = nullptr;
    }
    return *this;
}

// ═══════════════════════════════════════════════════
//  FILE LOADING
// ═══════════════════════════════════════════════════

bool DDSLoader::LoadFromFile(const char* filePath, DDSData& outData) {
    // ═══════════════════════════════════════════════════
    //  AUTO-DETECT FILE TYPE AND DISPATCH
    // ═══════════════════════════════════════════════════
    // The filePath is a VFS texture name like "ui\video_voroni_crop"
    // We try to find the file with different extensions:
    // 1. .dds → Static DDS texture
    // 2. .ogm → Theora video texture
    // 3. .avi → AVI video texture (future)
    //
    // We search in multiple VFS directories:
    // 1. $game_textures$ (general textures)
    // 2. $level$ (level-specific textures like build_details)
    //
    // NOTE: Config files often reference textures with .tga extension
    // (e.g., "fx\fx_sun.tga") but actual files are stored as .dds.
    // We strip any existing extension before searching.

    // Strip existing extension if present (e.g., "fx\fx_sun.tga" -> "fx\fx_sun")
    string_path basePath;
    xr_strcpy(basePath, filePath);
    if (char* dot = strrchr(basePath, '.')) {
        // Only strip if the extension looks like a texture extension
        if (_stricmp(dot, ".tga") == 0 ||
            _stricmp(dot, ".dds") == 0 ||
            _stricmp(dot, ".bmp") == 0 ||
            _stricmp(dot, ".png") == 0 ||
            _stricmp(dot, ".seq") == 0 ||
            _stricmp(dot, ".ogm") == 0 ||
            _stricmp(dot, ".avi") == 0) {
            *dot = '\0';  // Terminate string at the dot
        }
    }

    string_path resolvedPath;
    IReader* reader = nullptr;

    // Helper lambda to try finding file in VFS directories
    auto TryFindFile = [&](const char* ext) -> bool {
        // Try $game_textures$ first (most common)
        if (FS.exist(resolvedPath, "$game_textures$", basePath, ext)) {
            return true;
        }
        // Try $level$ (level-specific textures)
        if (FS.exist(resolvedPath, "$level$", basePath, ext)) {
            return true;
        }
        return false;
    };

    // Try DDS first (most common)
    if (TryFindFile(".dds")) {
        reader = FS.r_open(resolvedPath);

        if (!reader) {
            Msg("! [DDSLoader] Failed to open DDS file: %s", resolvedPath);
            outData.isValid = false;
            outData.errorMessage = "DDS file not found";
            return false;
        }

        // Continue with DDS loading below...
    }
    // Try SEQ (Animated sequence)
    else if (TryFindFile(".seq")) {
        // Msg("* [DDSLoader] Detected sequence texture: %s.seq", basePath);
        return LoadSequenceTexture(resolvedPath, outData);
    }
    // Try OGM (Theora video)
    else if (TryFindFile(".ogm")) {
        // Msg("* [DDSLoader] Detected video texture: %s.ogm", basePath);
        return LoadVideoTexture(resolvedPath, outData);
    }
    // Try AVI (future)
    else if (TryFindFile(".avi")) {
        Msg("! [DDSLoader] AVI video textures not yet supported: %s.avi", basePath);
        outData.isValid = false;
        outData.errorMessage = "AVI video textures not yet implemented";
        return false;
    }
    // Not found
    else {
        Msg("! [DDSLoader] Texture not found: %s (tried .dds, .seq, .ogm, .avi in $game_textures$ and $level$)", basePath);
        outData.isValid = false;
        outData.errorMessage = "Texture file not found";
        return false;
    }

    // ═══════════════════════════════════════════════════
    //  DDS LOADING (STATIC TEXTURE)
    // ═══════════════════════════════════════════════════

    // Read entire file into memory
    const u32 fileSize = reader->length();
    const u8* fileData = (const u8*)reader->pointer();

    // Copy file data so it stays valid after we close the reader
    outData.fileDataCopy.resize(fileSize);
    memcpy(outData.fileDataCopy.data(), fileData, fileSize);

    // Close file (safe now that we copied the data)
    FS.r_close(reader);

    // Parse from our owned copy
    bool success = LoadFromMemory(outData.fileDataCopy.data(), fileSize, outData, resolvedPath);

    // Store file path
    if (success) {
        outData.filePath = resolvedPath;
    }

    return success;
}

bool DDSLoader::LoadFromMemory(
    const u8* data,
    size_t dataSize,
    DDSData& outData,
    const char* debugName
) {
    // Validate input
    if (!data || dataSize < sizeof(u32) + sizeof(DDS_HEADER)) {
        Msg("! [DDSLoader] Invalid data (size=%zu, min=%zu)",
            dataSize, sizeof(u32) + sizeof(DDS_HEADER));
        outData.isValid = false;
        outData.errorMessage = "Invalid data size";
        return false;
    }

    // Check magic number
    const u32 magic = *(const u32*)data;
    if (magic != DDS_MAGIC) {
        Msg("! [DDSLoader] Invalid magic number: 0x%08X (expected 0x%08X)",
            magic, DDS_MAGIC);
        outData.isValid = false;
        outData.errorMessage = "Invalid DDS magic number";
        return false;
    }

    // Parse header
    DDS_HEADER header;
    DDS_HEADER_DX10 headerDX10;
    const u8* textureData = nullptr;
    size_t textureDataSize = 0;

    if (!ParseHeader(data, dataSize, header, &headerDX10, textureData, textureDataSize)) {
        Msg("! [DDSLoader] Failed to parse header: %s", debugName ? debugName : "unknown");
        outData.isValid = false;
        outData.errorMessage = "Header parsing failed";
        return false;
    }

    // Build TextureDesc from header
    TextureDesc& desc = outData.desc;

    // Determine texture type
    if (header.dwCaps2 & DDSCAPS2_CUBEMAP) {
        desc.type = TextureDesc::TextureCube;
        desc.arraySize = 6;  // Cubemap has 6 faces
    } else if (header.dwCaps2 & DDSCAPS2_VOLUME) {
        desc.type = TextureDesc::Texture3D;
        desc.depth = header.dwDepth > 0 ? header.dwDepth : 1;
    } else {
        desc.type = TextureDesc::Texture2D;
        desc.depth = 1;
    }

    // Dimensions
    desc.width = header.dwWidth;
    desc.height = header.dwHeight;

    // Mip levels
    if (header.dwFlags & DDSD_MIPMAPCOUNT) {
        desc.mipLevels = header.dwMipMapCount > 0 ? header.dwMipMapCount : 1;
    } else {
        desc.mipLevels = 1;
    }

    // Format conversion
    if (header.ddspf.dwFlags & DDPF_FOURCC) {
        if (header.ddspf.dwFourCC == FOURCC_DX10) {
            // Use DX10 header for format
            desc.format = GetFormatFromDXGI(headerDX10.dxgiFormat);
        } else {
            // Use FourCC for format
            desc.format = GetFormatFromFourCC(header.ddspf.dwFourCC);
        }
    } else {
        // Uncompressed format - detect from bit count and flags
        u32 bpp = header.ddspf.dwRGBBitCount;
        u32 flags = header.ddspf.dwFlags;

        // Detect format from bit depth and flags
        if (flags & DDPF_ALPHA) {
            // Alpha-only texture
            if (bpp == 8) {
                desc.format = nvrhi::Format::R8_UNORM;  // Use R8 for alpha
            } else {
                Msg("! [DDSLoader] Unsupported alpha format: %u bpp", bpp);
                desc.format = nvrhi::Format::UNKNOWN;
            }
        } else if (flags & DDPF_LUMINANCE) {
            // Luminance texture
            if (bpp == 8) {
                desc.format = nvrhi::Format::R8_UNORM;  // Luminance as R8
            } else if (bpp == 16 && (flags & DDPF_ALPHAPIXELS)) {
                desc.format = nvrhi::Format::RG8_UNORM;  // Luminance-Alpha as RG8
            } else {
                Msg("! [DDSLoader] Unsupported luminance format: %u bpp, flags=0x%X", bpp, flags);
                desc.format = nvrhi::Format::UNKNOWN;
            }
        } else if (flags & DDPF_RGB) {
            // RGB/RGBA texture
            if (bpp == 8) {
                desc.format = nvrhi::Format::R8_UNORM;
            } else if (bpp == 16) {
                if (flags & DDPF_ALPHAPIXELS) {
                    desc.format = nvrhi::Format::RG8_UNORM;  // Assume RG8
                } else {
                    desc.format = nvrhi::Format::R16_UNORM;  // Or R16
                }
            } else if (bpp == 32) {
                // Check channel masks to determine RGBA vs BGRA
                if (header.ddspf.dwRBitMask == 0x000000ff &&
                    header.ddspf.dwGBitMask == 0x0000ff00 &&
                    header.ddspf.dwBBitMask == 0x00ff0000 &&
                    header.ddspf.dwABitMask == 0xff000000) {
                    desc.format = nvrhi::Format::RGBA8_UNORM;
                } else if (header.ddspf.dwRBitMask == 0x00ff0000 &&
                           header.ddspf.dwGBitMask == 0x0000ff00 &&
                           header.ddspf.dwBBitMask == 0x000000ff &&
                           header.ddspf.dwABitMask == 0xff000000) {
                    desc.format = nvrhi::Format::BGRA8_UNORM;
                } else {
                    // Default to RGBA8
                    desc.format = nvrhi::Format::RGBA8_UNORM;
                }
            } else {
                Msg("! [DDSLoader] Unsupported RGB format: %u bpp", bpp);
                desc.format = nvrhi::Format::UNKNOWN;
            }
        } else {
            Msg("! [DDSLoader] Unknown uncompressed format: flags=0x%X, bpp=%u", flags, bpp);
            desc.format = nvrhi::Format::UNKNOWN;
        }
    }

    // Debug name
    if (debugName) {
        desc.debugName = debugName;
    }

    // Parse mipmap levels
    if (!ParseMipLevels(textureData, textureDataSize, desc, outData.mipLevels)) {
        Msg("! [DDSLoader] Failed to parse mip levels: %s", debugName ? debugName : "unknown");
        outData.isValid = false;
        outData.errorMessage = "Mipmap parsing failed";
        return false;
    }

    // Calculate total data size
    outData.totalDataSize = 0;
    for (const auto& mip : outData.mipLevels) {
        outData.totalDataSize += mip.size;
    }

    // Success
    outData.isValid = true;
    outData.errorMessage = "";

    return true;
}

// ═══════════════════════════════════════════════════
//  FORMAT CONVERSION
// ═══════════════════════════════════════════════════

nvrhi::Format DDSLoader::GetFormatFromFourCC(u32 fourCC) {
    switch (fourCC) {
        case FOURCC_DXT1:
            return nvrhi::Format::BC1_UNORM;

        case FOURCC_DXT3:
            return nvrhi::Format::BC2_UNORM;

        case FOURCC_DXT5:
            return nvrhi::Format::BC3_UNORM;

        case FOURCC_BC4U:
        case FOURCC_ATI1:
            return nvrhi::Format::BC4_UNORM;

        case FOURCC_BC4S:
            return nvrhi::Format::BC4_SNORM;

        case FOURCC_BC5U:
        case FOURCC_ATI2:
            return nvrhi::Format::BC5_UNORM;

        case FOURCC_BC5S:
            return nvrhi::Format::BC5_SNORM;

        default:
            Msg("! [DDSLoader] Unknown FourCC: 0x%08X (%.4s)", fourCC, (const char*)&fourCC);
            return nvrhi::Format::UNKNOWN;
    }
}

nvrhi::Format DDSLoader::GetFormatFromDXGI(DXGI_FORMAT dxgiFormat) {
    switch (dxgiFormat) {
        case DXGI_FORMAT_BC1_UNORM:
            return nvrhi::Format::BC1_UNORM;

        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return nvrhi::Format::BC1_UNORM_SRGB;

        case DXGI_FORMAT_BC2_UNORM:
            return nvrhi::Format::BC2_UNORM;

        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return nvrhi::Format::BC2_UNORM_SRGB;

        case DXGI_FORMAT_BC3_UNORM:
            return nvrhi::Format::BC3_UNORM;

        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return nvrhi::Format::BC3_UNORM_SRGB;

        case DXGI_FORMAT_BC4_UNORM:
            return nvrhi::Format::BC4_UNORM;

        case DXGI_FORMAT_BC4_SNORM:
            return nvrhi::Format::BC4_SNORM;

        case DXGI_FORMAT_BC5_UNORM:
            return nvrhi::Format::BC5_UNORM;

        case DXGI_FORMAT_BC5_SNORM:
            return nvrhi::Format::BC5_SNORM;

        case DXGI_FORMAT_BC6H_UF16:
            return nvrhi::Format::BC6H_UFLOAT;

        case DXGI_FORMAT_BC6H_SF16:
            return nvrhi::Format::BC6H_SFLOAT;

        case DXGI_FORMAT_BC7_UNORM:
            return nvrhi::Format::BC7_UNORM;

        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return nvrhi::Format::BC7_UNORM_SRGB;

        case DXGI_FORMAT_R32_FLOAT:
            return nvrhi::Format::R32_FLOAT;

        default:
            Msg("! [DDSLoader] Unknown DXGI format: %u", (u32)dxgiFormat);
            return nvrhi::Format::UNKNOWN;
    }
}

// ═══════════════════════════════════════════════════
//  HELPER METHODS
// ═══════════════════════════════════════════════════

u32 DDSLoader::CalculateMipSize(u32 width, u32 height, u32 depth, nvrhi::Format format) {
    u32 w = (width > 0) ? width : 1;
    u32 h = (height > 0) ? height : 1;
    u32 d = (depth > 0) ? depth : 1;

    u32 mipSize = 0;

    switch (format) {
        // BC1 (DXT1) - 4x4 blocks, 8 bytes per block
        case nvrhi::Format::BC1_UNORM:
        case nvrhi::Format::BC1_UNORM_SRGB:
        case nvrhi::Format::BC4_UNORM:
        case nvrhi::Format::BC4_SNORM:
            mipSize = ((w + 3) / 4) * ((h + 3) / 4) * 8;
            break;

        // BC2/BC3 (DXT3/DXT5), BC5, BC6H, BC7 - 4x4 blocks, 16 bytes per block
        case nvrhi::Format::BC2_UNORM:
        case nvrhi::Format::BC2_UNORM_SRGB:
        case nvrhi::Format::BC3_UNORM:
        case nvrhi::Format::BC3_UNORM_SRGB:
        case nvrhi::Format::BC5_UNORM:
        case nvrhi::Format::BC5_SNORM:
        case nvrhi::Format::BC6H_UFLOAT:
        case nvrhi::Format::BC6H_SFLOAT:
        case nvrhi::Format::BC7_UNORM:
        case nvrhi::Format::BC7_UNORM_SRGB:
            mipSize = ((w + 3) / 4) * ((h + 3) / 4) * 16;
            break;

        // Uncompressed formats
        case nvrhi::Format::RGBA8_UNORM:
        case nvrhi::Format::RGBA8_SNORM:
        case nvrhi::Format::SRGBA8_UNORM:
        case nvrhi::Format::BGRA8_UNORM:
        case nvrhi::Format::SBGRA8_UNORM:
            mipSize = w * h * 4;
            break;

        case nvrhi::Format::RGBA16_FLOAT:
        case nvrhi::Format::RGBA16_UNORM:
        case nvrhi::Format::RGBA16_SNORM:
            mipSize = w * h * 8;
            break;

        case nvrhi::Format::RGBA32_FLOAT:
            mipSize = w * h * 16;
            break;

        case nvrhi::Format::R8_UNORM:
            mipSize = w * h;
            break;

        case nvrhi::Format::RG8_UNORM:
            mipSize = w * h * 2;
            break;

        case nvrhi::Format::R16_UNORM:
        case nvrhi::Format::R16_SNORM:
        case nvrhi::Format::R16_FLOAT:
            mipSize = w * h * 2;
            break;

        default:
            Msg("! [DDSLoader] Unsupported format for size calculation: %d", (int)format);
            mipSize = w * h * 4;  // Fallback to 4 bytes per pixel
            break;
    }

    // Multiply by depth for 3D textures
    mipSize *= d;

    return mipSize;
}

void DDSLoader::CalculateMipDimensions(
    u32 baseWidth,
    u32 baseHeight,
    u32 baseDepth,
    u32 mipLevel,
    u32& outWidth,
    u32& outHeight,
    u32& outDepth
) {
    outWidth = (baseWidth >> mipLevel) > 0 ? (baseWidth >> mipLevel) : 1;
    outHeight = (baseHeight >> mipLevel) > 0 ? (baseHeight >> mipLevel) : 1;
    outDepth = (baseDepth >> mipLevel) > 0 ? (baseDepth >> mipLevel) : 1;
}

// ═══════════════════════════════════════════════════
//  INTERNAL PARSING
// ═══════════════════════════════════════════════════

bool DDSLoader::ParseHeader(
    const u8* data,
    size_t dataSize,
    DDS_HEADER& outHeader,
    DDS_HEADER_DX10* outHeaderDX10,
    const u8*& outTextureData,
    size_t& outTextureDataSize
) {
    // Skip magic number
    const u8* ptr = data + sizeof(u32);
    size_t remaining = dataSize - sizeof(u32);

    // Read DDS_HEADER
    if (remaining < sizeof(DDS_HEADER)) {
        Msg("! [DDSLoader] Not enough data for DDS_HEADER");
        return false;
    }

    memcpy(&outHeader, ptr, sizeof(DDS_HEADER));
    ptr += sizeof(DDS_HEADER);
    remaining -= sizeof(DDS_HEADER);

    // Validate header
    if (!ValidateHeader(outHeader)) {
        Msg("! [DDSLoader] Invalid DDS header");
        return false;
    }

    // Check for DX10 extension
    if (outHeader.ddspf.dwFlags & DDPF_FOURCC &&
        outHeader.ddspf.dwFourCC == FOURCC_DX10) {

        if (!outHeaderDX10) {
            Msg("! [DDSLoader] DX10 header required but not provided");
            return false;
        }

        if (remaining < sizeof(DDS_HEADER_DX10)) {
            Msg("! [DDSLoader] Not enough data for DDS_HEADER_DX10");
            return false;
        }

        memcpy(outHeaderDX10, ptr, sizeof(DDS_HEADER_DX10));
        ptr += sizeof(DDS_HEADER_DX10);
        remaining -= sizeof(DDS_HEADER_DX10);
    }

    // Texture data starts after headers
    outTextureData = ptr;
    outTextureDataSize = remaining;

    return true;
}

bool DDSLoader::ParseMipLevels(
    const u8* textureData,
    size_t textureDataSize,
    const TextureDesc& desc,
    xr_vector<DDSMipLevel>& outMipLevels
) {
    outMipLevels.clear();

    const u8* dataPtr = textureData;
    size_t remaining = textureDataSize;

    // For each array slice (or cubemap face)
    for (u32 arraySlice = 0; arraySlice < desc.arraySize; ++arraySlice) {
        // For each mip level
        for (u32 mipLevel = 0; mipLevel < desc.mipLevels; ++mipLevel) {
            u32 mipWidth, mipHeight, mipDepth;
            CalculateMipDimensions(
                desc.width, desc.height, desc.depth,
                mipLevel,
                mipWidth, mipHeight, mipDepth
            );

            u32 mipSize = CalculateMipSize(mipWidth, mipHeight, mipDepth, desc.format);

            if (remaining < mipSize) {
                Msg("! [DDSLoader] Not enough data for mip level %u (slice %u). Expected %u bytes, have %zu",
                    mipLevel, arraySlice, mipSize, remaining);
                return false;
            }

            DDSMipLevel mip;
            mip.data = dataPtr;
            mip.size = mipSize;
            mip.width = mipWidth;
            mip.height = mipHeight;
            mip.depth = mipDepth;

            // ═══════════════════════════════════════════════════
            //  CALCULATE PITCH VALUES FOR GPU UPLOAD
            // ═══════════════════════════════════════════════════

            const nvrhi::FormatInfo& formatInfo = nvrhi::getFormatInfo(desc.format);

            if (formatInfo.blockSize > 1) {
                // Block-compressed formats (BC1-BC7)
                // Blocks are 4x4 pixels, but we need to round up
                u32 blockWidth = (mipWidth + 3) / 4;
                u32 blockHeight = (mipHeight + 3) / 4;

                mip.rowPitch = blockWidth * formatInfo.bytesPerBlock;
                mip.slicePitch = mip.rowPitch * blockHeight;
            } else {
                // Uncompressed formats
                // CRITICAL: D3D11 requires rowPitch to be aligned to D3D11_TEXTURE_DATA_PITCH_ALIGNMENT (256 bytes for most hardware)
                // However, for small textures this is often ignored. We calculate the natural pitch first.
                u32 naturalRowPitch = mipWidth * formatInfo.bytesPerBlock;

                // For UpdateSubresource, we should use the natural pitch WITHOUT padding
                // The driver will handle alignment internally
                mip.rowPitch = naturalRowPitch;
                mip.slicePitch = mip.rowPitch * mipHeight;
            }

            // NOTE: For 3D textures, slicePitch is the stride between consecutive 2D slices,
            // NOT the total size of all depth slices. NVRHI/D3D11 use this as the pitch.
            // Do NOT multiply by mipDepth here!

            outMipLevels.push_back(mip);

            dataPtr += mipSize;
            remaining -= mipSize;
        }
    }

    return true;
}

bool DDSLoader::ValidateHeader(const DDS_HEADER& header) {
    // Check header size
    if (header.dwSize != 124) {
        Msg("! [DDSLoader] Invalid header size: %u (expected 124)", header.dwSize);
        return false;
    }

    // Check pixel format size
    if (header.ddspf.dwSize != 32) {
        Msg("! [DDSLoader] Invalid pixel format size: %u (expected 32)", header.ddspf.dwSize);
        return false;
    }

    // Check required flags
    if (!(header.dwFlags & DDSD_WIDTH) || !(header.dwFlags & DDSD_HEIGHT)) {
        Msg("! [DDSLoader] Missing required flags (DDSD_WIDTH | DDSD_HEIGHT)");
        return false;
    }

    // Check dimensions
    if (header.dwWidth == 0 || header.dwHeight == 0) {
        Msg("! [DDSLoader] Invalid dimensions: %ux%u", header.dwWidth, header.dwHeight);
        return false;
    }

    return true;
}

// ═══════════════════════════════════════════════════
//  PARTIAL MIP LOADING (OPTIMIZATION FOR STREAMING)
// ═══════════════════════════════════════════════════

bool DDSLoader::LoadMipRange(
    const char* filePath,
    u32 startMip,
    u32 mipCount,
    DDSData& outData)
{
    // ═══════════════════════════════════════════════════
    //  STEP 1: LOAD HEADER TO GET FORMAT INFO
    // ═══════════════════════════════════════════════════

    IReader* reader = FS.r_open(filePath);
    if (!reader) {
        Msg("! [DDSLoader] Failed to open file: %s", filePath);
        outData.isValid = false;
        outData.errorMessage = "File not found";
        return false;
    }

    const u32 fileSize = reader->length();
    if (fileSize < sizeof(u32) + sizeof(DDS_HEADER)) {
        FS.r_close(reader);
        Msg("! [DDSLoader] File too small: %s", filePath);
        outData.isValid = false;
        return false;
    }

    // Read magic number
    u32 magic;
    reader->r(&magic, sizeof(u32));
    if (magic != DDS_MAGIC) {
        FS.r_close(reader);
        Msg("! [DDSLoader] Invalid DDS magic: %s", filePath);
        outData.isValid = false;
        return false;
    }

    // Read header
    DDS_HEADER header;
    reader->r(&header, sizeof(DDS_HEADER));

    if (!ValidateHeader(header)) {
        FS.r_close(reader);
        outData.isValid = false;
        return false;
    }

    // Build texture description
    TextureDesc& desc = outData.desc;
    desc.width = header.dwWidth;
    desc.height = header.dwHeight;
    desc.mipLevels = (header.dwFlags & DDSD_MIPMAPCOUNT) ? header.dwMipMapCount : 1;

    // Determine texture type from DDS caps
    if (header.dwCaps2 & DDSCAPS2_CUBEMAP) {
        desc.type = TextureDesc::TextureCube;
        desc.arraySize = 6;
        desc.depth = 1;
    } else if (header.dwCaps2 & DDSCAPS2_VOLUME) {
        desc.type = TextureDesc::Texture3D;
        desc.arraySize = 1;
        desc.depth = header.dwDepth > 0 ? header.dwDepth : 1;
    } else {
        desc.type = TextureDesc::Texture2D;
        desc.arraySize = 1;
        desc.depth = 1;
    }

    // Determine format
    if (header.ddspf.dwFlags & DDPF_FOURCC) {
        desc.format = GetFormatFromFourCC(header.ddspf.dwFourCC);
    } else {
        // Simplified: assume RGBA8
        desc.format = nvrhi::Format::RGBA8_UNORM;
    }

    // ═══════════════════════════════════════════════════
    //  STEP 2: CALCULATE FILE OFFSET FOR START MIP
    // ═══════════════════════════════════════════════════

    u64 fileOffset = sizeof(u32) + sizeof(DDS_HEADER);
    u32 width = desc.width;
    u32 height = desc.height;
    u32 depth = desc.depth;

    // Skip mips before startMip (for all array slices)
    for (u32 arraySlice = 0; arraySlice < desc.arraySize; ++arraySlice) {
        for (u32 mip = 0; mip < startMip && mip < desc.mipLevels; ++mip) {
            u32 mipWidth = std::max(1u, width >> mip);
            u32 mipHeight = std::max(1u, height >> mip);
            u32 mipDepth = std::max(1u, depth >> mip);

            u32 mipSize = CalculateMipSize(mipWidth, mipHeight, mipDepth, desc.format);
            fileOffset += mipSize;
        }
    }

    // ═══════════════════════════════════════════════════
    //  STEP 3: CALCULATE TOTAL SIZE FOR MIP RANGE
    // ═══════════════════════════════════════════════════

    u64 totalSize = 0;
    for (u32 arraySlice = 0; arraySlice < desc.arraySize; ++arraySlice) {
        for (u32 mip = 0; mip < mipCount; ++mip) {
            u32 mipLevel = startMip + mip;
            if (mipLevel >= desc.mipLevels) break;

            u32 mipWidth = std::max(1u, width >> mipLevel);
            u32 mipHeight = std::max(1u, height >> mipLevel);
            u32 mipDepth = std::max(1u, depth >> mipLevel);

            u32 mipSize = CalculateMipSize(mipWidth, mipHeight, mipDepth, desc.format);
            totalSize += mipSize;
        }
    }

    // ═══════════════════════════════════════════════════
    //  STEP 4: READ MIP DATA FROM FILE
    // ═══════════════════════════════════════════════════

    reader->seek(fileOffset);

    outData.fileDataCopy.resize(totalSize);
    reader->r(outData.fileDataCopy.data(), totalSize);

    FS.r_close(reader);

    // ═══════════════════════════════════════════════════
    //  STEP 5: PARSE MIP LEVELS
    // ═══════════════════════════════════════════════════

    outData.mipLevels.clear();
    const u8* dataPtr = outData.fileDataCopy.data();

    for (u32 arraySlice = 0; arraySlice < desc.arraySize; ++arraySlice) {
        for (u32 mip = 0; mip < mipCount; ++mip) {
            u32 mipLevel = startMip + mip;
            if (mipLevel >= desc.mipLevels) break;

            u32 mipWidth = std::max(1u, width >> mipLevel);
            u32 mipHeight = std::max(1u, height >> mipLevel);
            u32 mipDepth = std::max(1u, depth >> mipLevel);

            DDSMipLevel mipData;
            mipData.data = dataPtr;
            mipData.width = mipWidth;
            mipData.height = mipHeight;
            mipData.depth = mipDepth;
            mipData.size = CalculateMipSize(mipWidth, mipHeight, mipDepth, desc.format);

            // Calculate pitch values
            const nvrhi::FormatInfo& formatInfo = nvrhi::getFormatInfo(desc.format);
            if (formatInfo.blockSize > 1) {
                u32 blockWidth = (mipWidth + 3) / 4;
                u32 blockHeight = (mipHeight + 3) / 4;
                mipData.rowPitch = blockWidth * formatInfo.bytesPerBlock;
                mipData.slicePitch = mipData.rowPitch * blockHeight;
            } else {
                mipData.rowPitch = mipWidth * formatInfo.bytesPerBlock;
                mipData.slicePitch = mipData.rowPitch * mipHeight;
            }

            // NOTE: For 3D textures, slicePitch is the stride between consecutive 2D slices,
            // NOT the total size of all depth slices. Do NOT multiply by mipDepth!

            outData.mipLevels.push_back(mipData);
            dataPtr += mipData.size;
        }
    }

    outData.totalDataSize = totalSize;
    outData.filePath = filePath;
    outData.isValid = true;

    // Msg("* [DDSLoader] LoadMipRange: %s (mips %u→%u, %llu KB)",
    //     filePath, startMip, startMip + mipCount, totalSize / 1024);

    return true;
}

// ═══════════════════════════════════════════════════
//  VIDEO TEXTURE LOADING (.OGM FORMAT)
// ═══════════════════════════════════════════════════

bool DDSLoader::LoadVideoTexture(const char* filePath, DDSData& outData) {
    // Msg("* [DDSLoader] Loading video texture: %s", filePath);

    // ═══════════════════════════════════════════════════
    //  STEP 1: LOAD THEORA STREAM AND GET METADATA
    // ═══════════════════════════════════════════════════

    CTheoraSurface* theoraSurface = xr_new<CTheoraSurface>();

    if (!theoraSurface->Load(filePath)) {
        Msg("! [DDSLoader] Failed to load Theora video: %s", filePath);
        xr_delete(theoraSurface);
        outData.isValid = false;
        outData.errorMessage = "Failed to open Theora video stream";
        return false;
    }

    // Get video dimensions
    u32 videoWidth = theoraSurface->Width(true);   // Actual video width
    u32 videoHeight = theoraSurface->Height(true); // Actual video height
    u32 texWidth = theoraSurface->Width(false);    // Pow2 texture width
    u32 texHeight = theoraSurface->Height(false);  // Pow2 texture height

    // Msg("* [DDSLoader] Video dimensions: %ux%u (texture: %ux%u)",
    //     videoWidth, videoHeight, texWidth, texHeight);

    // ═══════════════════════════════════════════════════
    //  STEP 2: CREATE VIDEO STATE
    // ═══════════════════════════════════════════════════

    outData.type = DDSData::TextureType::Video;
    outData.videoState = xr_new<DDSData::VideoState>();
    outData.videoState->theoraSurface = theoraSurface;
    outData.videoState->frameWidth = videoWidth;
    outData.videoState->frameHeight = videoHeight;
    outData.videoState->textureWidth = texWidth;   // Store padded dimensions for pitch calculation
    outData.videoState->textureHeight = texHeight;

    // Allocate frame buffer (RGBA8 format)
    u32 frameBufferSize = texWidth * texHeight;
    outData.videoState->frameBuffer.resize(frameBufferSize);

    // ═══════════════════════════════════════════════════
    //  STEP 3: SETUP TEXTURE DESCRIPTION
    // ═══════════════════════════════════════════════════

    outData.desc.type = TextureDesc::Texture2D;
    outData.desc.width = texWidth;
    outData.desc.height = texHeight;
    outData.desc.depth = 1;
    outData.desc.mipLevels = 1;  // Video textures don't have mipmaps
    outData.desc.arraySize = 1;
    outData.desc.format = nvrhi::Format::RGBA8_UNORM;  // Decoded to RGBA
    outData.desc.debugName = filePath;
    outData.desc.isRenderTarget = false;
    outData.desc.isUAV = false;

    // ═══════════════════════════════════════════════════
    //  STEP 4: CREATE DUMMY MIP LEVEL (FOR NVRHI CREATION)
    // ═══════════════════════════════════════════════════
    // We create one mip level pointing to our frame buffer
    // The actual data will be updated via writeTexture() each frame

    DDSMipLevel mip;
    mip.width = texWidth;
    mip.height = texHeight;
    mip.depth = 1;
    mip.size = frameBufferSize * 4;  // RGBA8 = 4 bytes per pixel
    mip.rowPitch = texWidth * 4;
    mip.slicePitch = mip.rowPitch * texHeight;
    mip.data = (const u8*)outData.videoState->frameBuffer.data();

    outData.mipLevels.push_back(mip);
    outData.totalDataSize = mip.size;

    // ═══════════════════════════════════════════════════
    //  STEP 5: START VIDEO PLAYBACK
    // ═══════════════════════════════════════════════════

    // Check if this is a looping video (menu videos loop, intros don't)
    bool shouldLoop = (strstr(filePath, "intro" DELIMITER) == nullptr) &&
                      (strstr(filePath, "outro" DELIMITER) == nullptr);

    theoraSurface->Play(shouldLoop, Device.dwTimeContinual);

    // ═══════════════════════════════════════════════════
    //  STEP 6: DECODE INITIAL FRAME
    // ═══════════════════════════════════════════════════
    // CRITICAL: We must decode the first frame BEFORE creating the GPU texture
    // Otherwise the texture will be created with uninitialized/black data!

    // Msg("* [DDSLoader] Decoding initial video frame...");

    // Update to get first frame
    bool firstFrameDecoded = theoraSurface->Update(Device.dwTimeContinual);
    if (!firstFrameDecoded) {
        // Force decode by waiting a bit
        Sleep(16); // Wait one frame
        firstFrameDecoded = theoraSurface->Update(Device.dwTimeContinual + 16);
    }

    if (firstFrameDecoded) {
        // Decompress first frame to our buffer
        int pos = 0;
        theoraSurface->DecompressFrame(
            outData.videoState->frameBuffer.data(),
            texWidth - videoWidth,  // Padding for pow2 textures
            pos
        );

        // Msg("* [DDSLoader] First frame decoded successfully (pos=%d, expected=%u)",
        //     pos, videoHeight * texWidth);
    } else {
        Msg("! [DDSLoader] WARNING: Failed to decode initial frame for: %s", filePath);
        // Fill with a visible color for debugging (magenta)
        for (u32& pixel : outData.videoState->frameBuffer) {
            pixel = 0xFFFF00FF;  // Magenta (ARGB)
        }
    }

    // Mark for GPU upload
    outData.videoState->needsUpdate = true;

    // ═══════════════════════════════════════════════════
    //  SUCCESS
    // ═══════════════════════════════════════════════════

    outData.isValid = true;
    outData.filePath = filePath;

    // Msg("* [DDSLoader] Video texture loaded: %s (%ux%u, loop=%s, initialFrame=%s)",
    //     filePath, videoWidth, videoHeight, shouldLoop ? "yes" : "no",
    //     firstFrameDecoded ? "OK" : "FAIL");

    return true;
}

// ═══════════════════════════════════════════════════
//  SEQUENCE TEXTURE LOADING (.SEQ FORMAT)
// ═══════════════════════════════════════════════════

bool DDSLoader::LoadSequenceTexture(const char* filePath, DDSData& outData) {
    // Msg("* [DDSLoader] Loading sequence texture: %s", filePath);

    // ═══════════════════════════════════════════════════
    //  STEP 1: PARSE .SEQ FILE
    // ═══════════════════════════════════════════════════

    IReader* reader = FS.r_open(filePath);
    if (!reader) {
        Msg("! [DDSLoader] Failed to open .seq file: %s", filePath);
        outData.isValid = false;
        outData.errorMessage = "Failed to open .seq file";
        return false;
    }

    string256 buffer;

    // Read first line - either "cycled" or FPS
    reader->r_string(buffer, sizeof(buffer));

    bool cycled = true;  // Default to looping (vanilla behavior)
    u32 fps = 0;

    if (0 == xr_stricmp(buffer, "cycled")) {
        // "cycled" means ping-pong animation (forward then reverse)
        // For now we treat it the same as regular looping
        cycled = true;
        reader->r_string(buffer, sizeof(buffer));
        fps = atoi(buffer);
    } else {
        fps = atoi(buffer);
    }

    if (fps == 0) {
        Msg("! [DDSLoader] Invalid FPS in .seq file: %s", filePath);
        FS.r_close(reader);
        outData.isValid = false;
        outData.errorMessage = "Invalid FPS in .seq file";
        return false;
    }

    // Read texture names (one per line)
    xr_vector<shared_str> frameTextureNames;
    while (!reader->eof()) {
        reader->r_string(buffer, sizeof(buffer));

        // Trim whitespace
        char* start = buffer;
        while (*start && isspace(*start)) start++;
        char* end = start + strlen(start) - 1;
        while (end > start && isspace(*end)) *end-- = '\0';

        // Skip empty lines
        if (*start == '\0') continue;

        frameTextureNames.push_back(shared_str(start));
    }

    FS.r_close(reader);

    if (frameTextureNames.empty()) {
        Msg("! [DDSLoader] No frames in .seq file: %s", filePath);
        outData.isValid = false;
        outData.errorMessage = "No frames in .seq file";
        return false;
    }

    // Msg("* [DDSLoader] Sequence FPS=%u, cycled=%s, frames=%u",
    //     fps, cycled ? "yes" : "no", (u32)frameTextureNames.size());

    // ═══════════════════════════════════════════════════
    //  STEP 2: PRE-LOAD ALL FRAME TEXTURES
    // ═══════════════════════════════════════════════════

    outData.type = DDSData::TextureType::Sequence;
    outData.sequenceState = xr_new<DDSData::SequenceState>();
    outData.sequenceState->fps = fps;
    outData.sequenceState->msPerFrame = 1000 / fps;
    outData.sequenceState->cycled = cycled;

    Msg("* [DDSLoader::LoadSequenceTexture] Initialized: fps=%u, msPerFrame=%u, cycled=%s, file=%s",
        fps, outData.sequenceState->msPerFrame, cycled ? "yes" : "no", filePath);

    u32 expectedWidth = 0;
    u32 expectedHeight = 0;
    nvrhi::Format expectedFormat = nvrhi::Format::UNKNOWN;

    for (u32 i = 0; i < frameTextureNames.size(); i++) {
        const char* frameName = frameTextureNames[i].c_str();

        // Msg("* [DDSLoader]   Loading frame %u/%u: %s",
        //     i + 1, (u32)frameTextureNames.size(), frameName);

        // Load frame DDS
        DDSData frameData;
        if (!LoadFromFile(frameName, frameData)) {
            Msg("! [DDSLoader] Failed to load frame texture: %s", frameName);
            xr_delete(outData.sequenceState);
            outData.isValid = false;
            outData.errorMessage = "Failed to load frame texture";
            return false;
        }

        // Validate frame dimensions match
        if (i == 0) {
            expectedWidth = frameData.desc.width;
            expectedHeight = frameData.desc.height;
            expectedFormat = frameData.desc.format;
        } else {
            if (frameData.desc.width != expectedWidth ||
                frameData.desc.height != expectedHeight ||
                frameData.desc.format != expectedFormat) {
                Msg("! [DDSLoader] Frame dimension mismatch: %s (%ux%u, format=%d) vs expected (%ux%u, format=%d)",
                    frameName, frameData.desc.width, frameData.desc.height, (int)frameData.desc.format,
                    expectedWidth, expectedHeight, (int)expectedFormat);
                xr_delete(outData.sequenceState);
                outData.isValid = false;
                outData.errorMessage = "Frame dimension mismatch";
                return false;
            }
        }

        // Store first mip level metadata and copy pixel data
        if (!frameData.mipLevels.empty()) {
            const DDSMipLevel& mip = frameData.mipLevels[0];

            // Copy metadata
            outData.sequenceState->frameData.push_back(mip);

            // Copy actual pixel data into our own buffer
            xr_vector<u8> pixelData;
            pixelData.resize(mip.size);
            memcpy(pixelData.data(), mip.data, mip.size);
            outData.sequenceState->framePixels.push_back(std::move(pixelData));

            // Update the frameData pointer to point to our owned data
            outData.sequenceState->frameData.back().data = outData.sequenceState->framePixels.back().data();
        }
    }

    // Msg("* [DDSLoader] Pre-loaded %u frames (%ux%u, format=%d)",
    //     (u32)outData.sequenceState->frameData.size(),
    //     expectedWidth, expectedHeight, (int)expectedFormat);

    // ═══════════════════════════════════════════════════
    //  STEP 3: CREATE SEQUENCE STATE (LIKE VIDEO STATE)
    // ═══════════════════════════════════════════════════

    outData.sequenceState->frameWidth = expectedWidth;
    outData.sequenceState->frameHeight = expectedHeight;
    outData.sequenceState->textureWidth = expectedWidth;
    outData.sequenceState->textureHeight = expectedHeight;

    // Allocate frame buffer (for current frame pixels)
    u32 frameBufferSize = expectedWidth * expectedHeight;
    outData.sequenceState->frameBuffer.resize(frameBufferSize);

    // ═══════════════════════════════════════════════════
    //  STEP 4: SETUP TEXTURE DESCRIPTION
    // ═══════════════════════════════════════════════════

    outData.desc.type = TextureDesc::Texture2D;
    outData.desc.width = expectedWidth;
    outData.desc.height = expectedHeight;
    outData.desc.depth = 1;
    outData.desc.mipLevels = 1;  // Sequence textures don't have mipmaps
    outData.desc.arraySize = 1;
    outData.desc.format = expectedFormat;
    outData.desc.debugName = filePath;
    outData.desc.isRenderTarget = false;
    outData.desc.isUAV = false;

    // ═══════════════════════════════════════════════════
    //  STEP 5: COPY FIRST FRAME TO FRAME BUFFER
    // ═══════════════════════════════════════════════════

    const DDSMipLevel& firstFrame = outData.sequenceState->frameData[0];

    // Copy first frame pixels to frame buffer
    // For RGBA8 formats, we can copy directly
    if (expectedFormat == nvrhi::Format::RGBA8_UNORM ||
        expectedFormat == nvrhi::Format::BGRA8_UNORM ||
        expectedFormat == nvrhi::Format::SRGBA8_UNORM ||
        expectedFormat == nvrhi::Format::SBGRA8_UNORM) {
        memcpy(outData.sequenceState->frameBuffer.data(),
               firstFrame.data,
               firstFrame.size);
    }

    // ═══════════════════════════════════════════════════
    //  STEP 6: CREATE DUMMY MIP LEVEL (FOR NVRHI CREATION)
    // ═══════════════════════════════════════════════════

    DDSMipLevel mip;
    mip.width = expectedWidth;
    mip.height = expectedHeight;
    mip.depth = 1;
    mip.size = firstFrame.size;
    mip.rowPitch = firstFrame.rowPitch;
    mip.slicePitch = firstFrame.slicePitch;
    mip.data = (const u8*)outData.sequenceState->frameBuffer.data();

    outData.mipLevels.push_back(mip);
    outData.totalDataSize = mip.size;

    // Mark for GPU upload
    outData.sequenceState->needsUpdate = true;

    // ═══════════════════════════════════════════════════
    //  SUCCESS
    // ═══════════════════════════════════════════════════

    outData.isValid = true;
    outData.filePath = filePath;

    // Msg("* [DDSLoader] Sequence texture loaded: %s (%ux%u, %u frames, %u fps, cycled=%s)",
    //     filePath, expectedWidth, expectedHeight,
    //     (u32)outData.sequenceState->frameData.size(), fps, cycled ? "yes" : "no");

    // Msg("* [DDSLoader] Sequence texture size: %ux%u from first frame",
    //     outData.desc.width, outData.desc.height);

    return true;
}

// ═══════════════════════════════════════════════════
//  VIDEO FRAME UPDATE
// ═══════════════════════════════════════════════════

bool DDSLoader::UpdateVideoFrame(DDSData& data, u32 currentTime) {
    // Verify this is a video texture
    if (data.type != DDSData::TextureType::Video || !data.videoState) {
        return false;
    }

    auto* videoState = data.videoState;
    auto* theora = videoState->theoraSurface;

    if (!theora) {
        return false;
    }

    // ═══════════════════════════════════════════════════
    //  DECODE NEXT FRAME
    // ═══════════════════════════════════════════════════

    bool frameChanged = theora->Update(currentTime);

    if (!frameChanged) {
        return false;  // Same frame, no update needed
    }

    // ═══════════════════════════════════════════════════
    //  DECOMPRESS FRAME TO RGBA BUFFER
    // ═══════════════════════════════════════════════════

    u32 texWidth = theora->Width(false);
    u32 videoWidth = theora->Width(true);

    int pos = 0;
    theora->DecompressFrame(
        videoState->frameBuffer.data(),
        texWidth - videoWidth,  // Padding for pow2 textures
        pos
    );

    // Mark as needing GPU upload
    videoState->needsUpdate = true;

    return true;
}

// ═══════════════════════════════════════════════════
//  SEQUENCE FRAME UPDATE
// ═══════════════════════════════════════════════════

bool DDSLoader::UpdateSequenceFrame(DDSData& data, u32 currentFrame) {
    // Verify this is a sequence texture
    if (data.type != DDSData::TextureType::Sequence || !data.sequenceState) {
        return false;
    }

    auto* seqState = data.sequenceState;

    // Validate frame index
    if (currentFrame >= seqState->frameData.size()) {
        return false;
    }

    // ═══════════════════════════════════════════════════
    //  COPY FRAME PIXELS TO FRAME BUFFER
    // ═══════════════════════════════════════════════════

    const DDSMipLevel& frame = seqState->frameData[currentFrame];

    // Copy frame pixels to frame buffer
    // For both compressed and uncompressed formats, we just copy the raw bytes
    // The GPU texture was already created with the correct format, so it expects this data layout

    // Ensure frame buffer is sized correctly for this frame
    u32 requiredSize = (frame.size + sizeof(u32) - 1) / sizeof(u32);  // Round up to u32 count
    if (seqState->frameBuffer.size() < requiredSize) {
        seqState->frameBuffer.resize(requiredSize);
    }

    memcpy(seqState->frameBuffer.data(), frame.data, frame.size);

    // Mark as needing GPU upload
    seqState->needsUpdate = true;

    return true;
}

} // namespace xray::render::resources
