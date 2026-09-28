#include "DdsTexture.h"

#include <gli/convert.hpp>
#include <gli/load_dds.hpp>

#include <algorithm>
#include <cstring>
#include <utility>

namespace xray::render::vulkan
{
namespace
{
uint32_t read32(const std::uint8_t* bytes, size_t offset)
{
    return uint32_t(bytes[offset]) | (uint32_t(bytes[offset + 1]) << 8) |
        (uint32_t(bytes[offset + 2]) << 16) | (uint32_t(bytes[offset + 3]) << 24);
}

VkFormat vk_format(gli::format format)
{
    switch (format)
    {
    case gli::FORMAT_RGBA8_UNORM_PACK8: return VK_FORMAT_R8G8B8A8_UNORM;
    case gli::FORMAT_BGRA8_UNORM_PACK8: return VK_FORMAT_B8G8R8A8_UNORM;
    case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8: return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
    case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16: return VK_FORMAT_BC2_UNORM_BLOCK;
    case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16: return VK_FORMAT_BC2_SRGB_BLOCK;
    case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16: return VK_FORMAT_BC3_UNORM_BLOCK;
    case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16: return VK_FORMAT_BC3_SRGB_BLOCK;
    default: return VK_FORMAT_UNDEFINED;
    }
}

bool compressed(VkFormat format)
{
    return format >= VK_FORMAT_BC1_RGB_UNORM_BLOCK && format <= VK_FORMAT_BC3_SRGB_BLOCK;
}
}

bool decode_dds(const void* data, size_t size, bool bc_supported, DdsTexture& result, std::string& error)
{
    result = {};
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    if (!bytes || size < 128 || std::memcmp(bytes, "DDS ", 4) != 0 || read32(bytes, 4) != 124 ||
        read32(bytes, 76) != 32)
    {
        error = "invalid DDS header";
        return false;
    }

    const uint32_t width = read32(bytes, 16), height = read32(bytes, 12);
    const uint32_t mip_count = (read32(bytes, 8) & 0x20000u) ? read32(bytes, 28) : 1;
    if (!width || !height || width > 16384 || height > 16384 || !mip_count || mip_count > 15 ||
        (read32(bytes, 112) & 0x200u) || (read32(bytes, 8) & 0x800000u))
    {
        error = "unsupported DDS dimensions or target";
        return false;
    }

    const uint32_t fourcc = read32(bytes, 84);
    const bool extended = fourcc == 0x30315844u; // DX10
    const size_t header_size = extended ? 148 : 128;
    if (size < header_size || (extended && (read32(bytes, 140) != 1 || read32(bytes, 136) != 3)))
    {
        error = "unsupported DDS extension";
        return false;
    }

    // GLI's pinned loader asserts on truncated payloads. Validate its expected layout first.
    const uint32_t dxgi = extended ? read32(bytes, 128) : 0;
    uint32_t block_bytes = 0;
    if (fourcc == 0x31545844u || dxgi == 71 || dxgi == 72) block_bytes = 8; // BC1
    else if (fourcc == 0x33545844u || fourcc == 0x35545844u ||
        dxgi == 74 || dxgi == 75 || dxgi == 77 || dxgi == 78) block_bytes = 16; // BC2/BC3
    else if (fourcc != 0 && (!extended || (dxgi != 28 && dxgi != 29 && dxgi != 87 && dxgi != 91)))
    {
        error = "unsupported DDS FourCC";
        return false;
    }
    const uint32_t bits = read32(bytes, 88);
    if (!block_bytes && !extended && (bits != 32 ||
        !(read32(bytes, 92) == 0x000000ffu && read32(bytes, 96) == 0x0000ff00u &&
          read32(bytes, 100) == 0x00ff0000u && read32(bytes, 104) == 0xff000000u) &&
        !(read32(bytes, 92) == 0x00ff0000u && read32(bytes, 96) == 0x0000ff00u &&
          read32(bytes, 100) == 0x000000ffu && read32(bytes, 104) == 0xff000000u)))
    {
        error = "unsupported DDS pixel format";
        return false;
    }

    size_t expected = header_size;
    for (uint32_t level = 0; level < mip_count; ++level)
    {
        const size_t w = std::max(1u, width >> level), h = std::max(1u, height >> level);
        expected += block_bytes ? ((w + 3) / 4) * ((h + 3) / 4) * block_bytes : w * h * 4;
    }
    if (expected != size || expected > 256u * 1024u * 1024u)
    {
        error = "DDS mip payload has an invalid size";
        return false;
    }

    const auto* characters = reinterpret_cast<const char*>(bytes);
    gli::texture loaded = gli::load_dds(characters, size);
    if (loaded.empty() || loaded.target() != gli::TARGET_2D || loaded.layers() != 1 || loaded.faces() != 1 ||
        loaded.levels() != mip_count || static_cast<uint32_t>(loaded.extent().x) != width ||
        static_cast<uint32_t>(loaded.extent().y) != height)
    {
        error = "DDS loader rejected texture";
        return false;
    }
    VkFormat format = vk_format(loaded.format());
    if (format == VK_FORMAT_UNDEFINED)
    {
        error = "DDS texture format has no Vulkan mapping";
        return false;
    }

    if (!bc_supported && compressed(format))
    {
        loaded = gli::convert(gli::texture2d(loaded), gli::FORMAT_RGBA8_UNORM_PACK8);
        format = VK_FORMAT_R8G8B8A8_UNORM;
    }

    DdsTexture decoded;
    decoded.format = format;
    decoded.extent = { width, height, 1 };
    for (uint32_t level = 0; level < mip_count; ++level)
    {
        const size_t offset = (decoded.pixels.size() + 3u) & ~size_t(3u);
        decoded.pixels.resize(offset, 0);
        VkBufferImageCopy copy{};
        copy.bufferOffset = offset;
        copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1 };
        copy.imageExtent = { std::max(1u, width >> level), std::max(1u, height >> level), 1 };
        decoded.copies.push_back(copy);
        const auto* src = static_cast<const std::uint8_t*>(loaded.data(0, 0, level));
        decoded.pixels.insert(decoded.pixels.end(), src, src + loaded.size(level));
    }
    result = std::move(decoded);
    error.clear();
    return true;
}
}
