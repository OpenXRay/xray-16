#include "src/Layers/xrRenderVK/DdsTexture.h"
#include <gli/texture.hpp>
#include <gli/save_dds.hpp>
#include <cassert>
#include <cstring>
#include <vector>
using namespace xray::render::vulkan;
namespace
{
void set32(std::vector<char>& bytes, size_t offset, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = static_cast<char>(value >> (8 * i));
}
std::vector<char> legacy_dds(uint32_t flags, uint32_t bits, uint32_t red,
    uint32_t green, uint32_t blue, uint32_t alpha, const std::vector<uint8_t>& pixels)
{
    std::vector<char> bytes(128 + pixels.size());
    std::memcpy(bytes.data(), "DDS ", 4);
    set32(bytes, 4, 124);
    set32(bytes, 8, 0x100fu);
    set32(bytes, 12, 1);
    set32(bytes, 16, static_cast<uint32_t>(pixels.size() * 8 / bits));
    set32(bytes, 76, 32);
    set32(bytes, 80, flags);
    set32(bytes, 88, bits);
    set32(bytes, 92, red);
    set32(bytes, 96, green);
    set32(bytes, 100, blue);
    set32(bytes, 104, alpha);
    std::memcpy(bytes.data() + 128, pixels.data(), pixels.size());
    return bytes;
}
}
int main()
{
    for (bool cube : {false, true})
    for (auto format : {gli::FORMAT_RGBA8_UNORM_PACK8, gli::FORMAT_RGBA8_SRGB_PACK8,
        gli::FORMAT_BGRA8_UNORM_PACK8, gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16,
        gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16})
    {
        gli::texture original(cube ? gli::TARGET_CUBE : gli::TARGET_2D, format, {8,8,1}, 1, cube ? 6 : 1, 4);
        for (size_t face=0; face<original.faces(); ++face)
            for (size_t level=0; level<4; ++level)
                std::memset(original.data(0,face,level), int(face*16+level), original.size(level));
        std::vector<char> bytes;
        assert(gli::save_dds(original,bytes));
        DdsTexture decoded;
        std::string error;
        assert(decode_dds(bytes.data(),bytes.size(),true,decoded,error));
        assert(decoded.cube==cube && decoded.mip_levels==4 && decoded.layers==original.faces());
        assert(decoded.copies.size()==original.faces()*4);
        for (const auto& copy : decoded.copies)
        {
            const auto level=copy.imageSubresource.mipLevel, face=copy.imageSubresource.baseArrayLayer;
            assert(!std::memcmp(decoded.pixels.data()+copy.bufferOffset, original.data(0,face,level), original.size(level)));
        }
        assert(!decode_dds(bytes.data(),bytes.size()-1,true,decoded,error));
        if (format==gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16 || format==gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16)
        {
            assert(decode_dds(bytes.data(),bytes.size(),false,decoded,error));
            assert(decoded.format==(format==gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16 ?
                VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM));
            assert(decoded.copies.size()==original.faces()*4);
        }
    }
    for (const auto& sample : {
        legacy_dds(0x41, 16, 0x0f00, 0x00f0, 0x000f, 0xf000, {0x0f, 0xf0}),
        legacy_dds(0x40, 16, 0xf800, 0x07e0, 0x001f, 0, {0x00, 0xf8}),
        legacy_dds(0x40, 24, 0xff0000, 0x00ff00, 0x0000ff, 0, {0x30, 0x20, 0x10}),
        legacy_dds(0x2, 8, 0, 0, 0, 0xff, {0x80}),
        legacy_dds(0x20000, 8, 0xff, 0, 0, 0, {0x80})})
    {
        DdsTexture decoded;
        std::string error;
        assert(decode_dds(sample.data(), sample.size(), true, decoded, error));
        assert(decoded.format == VK_FORMAT_R8G8B8A8_UNORM && decoded.copies.size() == 1);
        assert(decoded.pixels.size() == 4);
        assert(!decode_dds(sample.data(), sample.size() - 1, true, decoded, error));
    }
    auto font = legacy_dds(0x2, 8, 0, 0, 0, 0xff, {0x80});
    DdsTexture decoded;
    std::string error;
    assert(decode_dds(font.data(), font.size(), true, decoded, error));
    assert((decoded.pixels == std::vector<uint8_t>{255, 255, 255, 128}));
    auto rgb565 = legacy_dds(0x40, 16, 0xf800, 0x07e0, 0x001f, 0, {0x00, 0xf8});
    assert(decode_dds(rgb565.data(), rgb565.size(), true, decoded, error));
    assert((decoded.pixels == std::vector<uint8_t>{255, 0, 0, 255}));
    auto padded = legacy_dds(0x20000, 8, 0xff, 0, 0, 0, {10, 20, 30, 0});
    set32(padded, 16, 3);
    assert(decode_dds(padded.data(), padded.size(), true, decoded, error));
    assert(decoded.pixels.size() == 12 && decoded.pixels[0] == 10 && decoded.pixels[8] == 30);
    set32(rgb565, 104, 0xf800); // Overlapping color and alpha masks are invalid.
    assert(!decode_dds(rgb565.data(), rgb565.size(), true, decoded, error));
}
