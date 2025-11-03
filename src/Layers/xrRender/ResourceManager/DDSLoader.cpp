#include "stdafx.h"
#include "DDSLoader.h"
#include "xrCore/FS.h"

// DDS File Format Loader Implementation
// Week 1 - Day 2: Task 2.1

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  FILE LOADING
// ═══════════════════════════════════════════════════

bool DDSLoader::LoadFromFile(const char* filePath, DDSData& outData) {
    // The filePath may be:
    // 1. A VFS path like "$game_textures$\texture_name"
    // 2. A relative path like "textures\texture_name"
    // 3. An absolute path

    string_path resolvedPath;
    IReader* reader = nullptr;

    // Try to resolve through VFS if it looks like a texture path
    if (strstr(filePath, "$game_textures$")) {
        // Extract just the filename part after $game_textures$
        const char* nameStart = strstr(filePath, "$game_textures$");
        nameStart += strlen("$game_textures$");
        while (*nameStart == '\\' || *nameStart == '/') nameStart++;  // Skip path separators

        // Remove .dds extension if present
        string_path textureName;
        xr_strcpy(textureName, nameStart);
        if (strstr(textureName, ".dds")) {
            *strstr(textureName, ".dds") = 0;
        }

        // Try to find through VFS
        if (FS.exist(resolvedPath, "$game_textures$", textureName, ".dds")) {
            reader = FS.r_open(resolvedPath);
        }
    } else {
        // Try direct open for absolute/relative paths
        reader = FS.r_open(filePath);
        if (reader) {
            xr_strcpy(resolvedPath, filePath);
        }
    }

    if (!reader) {
        Msg("! [DDSLoader] Failed to open file: %s", filePath);
        outData.isValid = false;
        outData.errorMessage = "File not found";
        return false;
    }

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
        // Uncompressed RGB/RGBA format
        // For now, assume RGBA8_UNORM (can be extended)
        desc.format = nvrhi::Format::RGBA8_UNORM;
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

    Msg("* [DDSLoader] Loaded: %s (%ux%u, %u mips, format=%d, %llu bytes)",
        debugName ? debugName : "unknown",
        desc.width, desc.height, desc.mipLevels,
        (int)desc.format, outData.totalDataSize);

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

} // namespace xray::render::resources
