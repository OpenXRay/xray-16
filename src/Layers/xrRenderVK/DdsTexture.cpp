#include "DdsTexture.h"

#include <gli/convert.hpp>
#include <gli/load_dds.hpp>

#include <algorithm>
#include <cstring>
#include <new>
#include <sstream>
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
    case gli::FORMAT_RGBA8_SRGB_PACK8: return VK_FORMAT_R8G8B8A8_SRGB;
    case gli::FORMAT_BGRA8_SRGB_PACK8: return VK_FORMAT_B8G8R8A8_SRGB;
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

uint8_t channel(uint32_t pixel, uint32_t mask)
{
    if (!mask) return 0;
    unsigned shift = 0;
    while (!(mask & 1u)) { mask >>= 1; ++shift; }
    const uint64_t value = (pixel >> shift) & mask;
    return static_cast<uint8_t>((value * 255u + mask / 2u) / mask);
}

bool contiguous(uint32_t mask)
{
    if (!mask) return true;
    while (!(mask & 1u)) mask >>= 1;
    return (mask & (mask + 1u)) == 0;
}

bool decode_legacy_pixels(const uint8_t* bytes, size_t size, uint32_t width,
    uint32_t height, uint32_t mip_count, uint32_t faces, DdsTexture& result,
    std::string& error)
{
    const uint32_t flags = read32(bytes, 80);
    const uint32_t bits = read32(bytes, 88);
    const bool rgb = (flags & 0x40u) != 0;
    const bool luminance = (flags & 0x20000u) != 0;
    const bool alpha_only = (flags & 0x2u) != 0 && !rgb && !luminance;
    uint32_t red = read32(bytes, 92), green = read32(bytes, 96);
    uint32_t blue = read32(bytes, 100), alpha = read32(bytes, 104);
    if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
    {
        error = "unsupported DDS pixel bit count: " + std::to_string(bits);
        return false;
    }
    const uint32_t valid_bits = bits == 32 ? ~0u : (1u << bits) - 1u;
    if ((luminance || alpha_only) && !red && !alpha)
        (alpha_only ? alpha : red) = valid_bits;
    const bool valid_masks = rgb ? red && green && blue : (luminance ? red != 0 : alpha_only && alpha != 0);
    if (!valid_masks || ((red | green | blue | alpha) & ~valid_bits) ||
        (red & green) || (red & blue) || (red & alpha) ||
        (green & blue) || (green & alpha) || (blue & alpha) ||
        !contiguous(red) || !contiguous(green) || !contiguous(blue) || !contiguous(alpha))
    {
        std::ostringstream detail;
        detail << "unsupported DDS pixel masks flags=" << flags << " bits=" << bits <<
            " masks=" << red << "/" << green << "/" << blue << "/" << alpha;
        error = detail.str();
        return false;
    }

    // Writers disagree about whether uncompressed mip rows are byte- or
    // DWORD-aligned. Select the layout matching the actual payload length.
    unsigned alignment = 0;
    for (const unsigned candidate : {1u, 4u})
    {
        size_t expected = 128;
        for (uint32_t level = 0; level < mip_count; ++level)
        {
            const size_t row = (size_t(std::max(1u, width >> level)) * bits + 7u) / 8u;
            expected += ((row + candidate - 1u) / candidate) * candidate * std::max(1u, height >> level) * faces;
        }
        if (expected == size && expected <= 256u * 1024u * 1024u)
        { alignment = candidate; break; }
    }
    if (!alignment)
    {
        error = "DDS mip payload has an invalid size";
        return false;
    }

    DdsTexture decoded;
    decoded.format = VK_FORMAT_R8G8B8A8_UNORM;
    decoded.extent = {width, height, 1};
    decoded.mip_levels = mip_count;
    decoded.layers = faces;
    decoded.cube = faces == 6;
    size_t source = 128;
    for (uint32_t face = 0; face < faces; ++face)
    for (uint32_t level = 0; level < mip_count; ++level)
    {
        const uint32_t w = std::max(1u, width >> level), h = std::max(1u, height >> level);
        const size_t row = (size_t(w) * bits + 7u) / 8u;
        const size_t stride = (row + alignment - 1u) / alignment * alignment;
        VkBufferImageCopy copy{};
        copy.bufferOffset = decoded.pixels.size();
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, face, 1};
        copy.imageExtent = {w, h, 1};
        decoded.copies.push_back(copy);
        decoded.pixels.reserve(decoded.pixels.size() + size_t(w) * h * 4u);
        for (uint32_t y = 0; y < h; ++y)
        for (uint32_t x = 0; x < w; ++x)
        {
            const size_t index = source + size_t(y) * stride + size_t(x) * (bits / 8u);
            uint32_t pixel = 0;
            for (uint32_t i = 0; i < bits / 8u; ++i)
                pixel |= uint32_t(bytes[index + i]) << (8u * i);
            const uint8_t color = channel(pixel, red);
            decoded.pixels.push_back(alpha_only ? 255 : color);
            decoded.pixels.push_back(luminance ? color : alpha_only ? 255 : channel(pixel, green));
            decoded.pixels.push_back(luminance ? color : alpha_only ? 255 : channel(pixel, blue));
            decoded.pixels.push_back(alpha ? channel(pixel, alpha) : 255);
        }
        source += stride * h;
    }
    result = std::move(decoded);
    error.clear();
    return true;
}
}

bool decode_dds(const void* data, size_t size, bool bc_supported, DdsTexture& result, std::string& error) try
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
        (read32(bytes, 8) & 0x800000u))
    {
        error = "unsupported DDS dimensions or target";
        return false;
    }

    const uint32_t fourcc = (read32(bytes, 80) & 0x4u) ? read32(bytes, 84) : 0;
    const bool extended = fourcc == 0x30315844u; // DX10
    const size_t header_size = extended ? 148 : 128;
    if (size < header_size || (extended && (read32(bytes, 140) != 1 || read32(bytes, 132) != 3)))
    {
        error = "unsupported DDS extension";
        return false;
    }

    const uint32_t cube_caps = read32(bytes, 112) & 0xfe00u;
    const bool cube = extended ? (read32(bytes, 136) & 4u) != 0 : (cube_caps & 0x200u) != 0;
    if ((cube && width != height) || (!extended && cube_caps && cube_caps != 0xfe00u))
    {
        error = "DDS cubemap requires six square faces";
        return false;
    }
    const uint32_t faces = cube ? 6 : 1;

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
        return decode_legacy_pixels(bytes, size, width, height, mip_count, faces, result, error);
    }

    size_t expected = header_size;
    for (uint32_t level = 0; level < mip_count; ++level)
    {
        const size_t w = std::max(1u, width >> level), h = std::max(1u, height >> level);
        expected += block_bytes ? ((w + 3) / 4) * ((h + 3) / 4) * block_bytes : w * h * 4;
    }
    expected = header_size + (expected - header_size) * faces;
    if (expected != size || expected > 256u * 1024u * 1024u)
    {
        error = "DDS mip payload has an invalid size";
        return false;
    }

    const auto* characters = reinterpret_cast<const char*>(bytes);
    gli::texture loaded = gli::load_dds(characters, size);
    if (loaded.empty() || loaded.target() != (cube ? gli::TARGET_CUBE : gli::TARGET_2D) || loaded.layers() != 1 || loaded.faces() != faces ||
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
        const bool srgb = gli::is_srgb(loaded.format());
        const auto decodedFormat = srgb ? gli::FORMAT_RGBA8_SRGB_PACK8 : gli::FORMAT_RGBA8_UNORM_PACK8;
        loaded = cube ? gli::texture(gli::convert(gli::texture_cube(loaded), decodedFormat)) :
            gli::texture(gli::convert(gli::texture2d(loaded), decodedFormat));
        if (loaded.empty() || loaded.layers() != 1 || loaded.faces() != faces ||
            loaded.levels() != mip_count)
        {
            error = "DDS decompression did not produce all faces and mips";
            return false;
        }
        format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    }

    DdsTexture decoded;
    decoded.format = format;
    decoded.extent = { width, height, 1 };
    decoded.mip_levels = mip_count;
    decoded.layers = faces;
    decoded.cube = cube;
    // Copy each mip into one allocation. Repeated vector growth temporarily
    // keeps both the old and new buffers resident on 32-bit Android.
    const size_t alignment_bytes = size_t(faces) * mip_count * 3u;
    if (loaded.size() > decoded.pixels.max_size() - alignment_bytes)
    {
        error = "DDS decoded payload is too large";
        return false;
    }
    decoded.pixels.reserve(loaded.size() + alignment_bytes);
    decoded.copies.reserve(size_t(faces) * mip_count);
    for (uint32_t face = 0; face < faces; ++face)
    for (uint32_t level = 0; level < mip_count; ++level)
    {
        const size_t offset = (decoded.pixels.size() + 3u) & ~size_t(3u);
        decoded.pixels.resize(offset, 0);
        VkBufferImageCopy copy{};
        copy.bufferOffset = offset;
        copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level, face, 1 };
        copy.imageExtent = { std::max(1u, width >> level), std::max(1u, height >> level), 1 };
        decoded.copies.push_back(copy);
        const size_t bytes = loaded.size(level);
        const size_t w = copy.imageExtent.width, h = copy.imageExtent.height;
        const size_t expected_mip = compressed(format) ?
            ((w + 3) / 4) * ((h + 3) / 4) * block_bytes : w * h * 4;
        const auto extent = loaded.extent(level);
        if (!bytes || bytes != expected_mip || extent.x != w || extent.y != h)
        {
            error = "DDS face/mip layout is inconsistent: face=" + std::to_string(face) +
                " level=" + std::to_string(level);
            return false;
        }
        const auto* src = static_cast<const std::uint8_t*>(loaded.data(0, face, level));
        if (!src)
        {
            error = "DDS face/mip data is missing: face=" + std::to_string(face) +
                " level=" + std::to_string(level);
            return false;
        }
        decoded.pixels.insert(decoded.pixels.end(), src, src + bytes);
    }
    result = std::move(decoded);
    error.clear();
    return true;
}
catch (const std::bad_alloc&)
{
    result = {};
    error = "DDS OOM";
    return false;
}
}
