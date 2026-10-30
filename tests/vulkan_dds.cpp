#include "src/Layers/xrRenderVK/DdsTexture.h"
#include <gli/texture.hpp>
#include <gli/save_dds.hpp>
#include <gli/convert.hpp>
#include <cassert>
#include <cstring>
#include <cstdio>
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
    // Compare complete BC fallback payloads against the independent texel
    // conversion path, including small/partial blocks, faces and mip tails.
    for(unsigned dimension : {7u,8u})
    for(bool cube : {false,true})
    for(auto unorm : {gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8,
            gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16,gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16})
    for(bool srgb : {false,true})
    {
        const auto format=gli::format(int(unorm)+(srgb?1:0));
        gli::texture input(cube?gli::TARGET_CUBE:gli::TARGET_2D,format,{int(dimension),int(dimension),1},1,cube?6:1,3);
        auto* bytes=static_cast<uint8_t*>(input.data());uint32_t random=357;
        for(size_t i=0;i<input.size();++i) { random=random*1664525+1013904223;bytes[i]=uint8_t(random>>24); }
        std::vector<char> file;assert(gli::save_dds(input,file));
        DdsTexture decoded;std::string error;
        if(!decode_dds(file.data(),file.size(),false,decoded,error))
        { std::fprintf(stderr,"BC fixture cube=%d format=%d bytes=%zu: %s\n",cube,int(format),file.size(),error.c_str());assert(false); }
        // Compressed sRGB blocks hold the same encoded bytes as UNORM.
        // Decode palettes without adding another sRGB encoding operation.
        for(const auto& copy:decoded.copies) {
            const auto level=copy.imageSubresource.mipLevel,face=copy.imageSubresource.baseArrayLayer;
            // GLI's per-texel path rounds block strides down for NPOT. Use
            // an equivalent padded source to compare every visible texel.
            const size_t w=copy.imageExtent.width,h=copy.imageExtent.height;
            const size_t pw=(w+3)/4*4,ph=(h+3)/4*4;
            gli::texture padded(gli::TARGET_2D,unorm,{int(pw),int(ph),1},1,1,1);
            assert(padded.size()==input.size(level));
            std::memcpy(padded.data(),input.data(0,face,level),input.size(level));
            const auto reference=gli::convert(gli::texture2d(padded),gli::FORMAT_RGBA8_UNORM_PACK8);
            const auto* expected=static_cast<const uint8_t*>(reference.data());
            const auto* actual=decoded.pixels.data()+copy.bufferOffset;
            for(size_t y=0;y<h;++y) for(size_t x=0;x<w;++x) for(size_t c=0;c<4;++c)
                assert(actual[(y*w+x)*4+c]==expected[(y*pw+x)*4+c]);
            assert(decoded.format==(srgb?VK_FORMAT_R8G8B8A8_SRGB:VK_FORMAT_R8G8B8A8_UNORM));
        }
    }
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
        DdsTexture reduced;
        assert(decode_dds(bytes.data(),bytes.size(),true,reduced,error,2));
        assert(reduced.extent.width==(cube?8u:2u) && reduced.mip_levels==(cube?4u:2u));
        for(const auto& copy:reduced.copies) {
            const unsigned mip=copy.imageSubresource.mipLevel+(cube?0:2);
            const unsigned face=copy.imageSubresource.baseArrayLayer;
            assert(!std::memcmp(reduced.pixels.data()+copy.bufferOffset,original.data(0,face,mip),original.size(mip)));
        }
        assert(decode_dds(bytes.data(),bytes.size(),false,reduced,error,2));
        assert(reduced.extent.width==(cube?8u:2u) && reduced.mip_levels==(cube?4u:2u));
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
    // Real-sized BC fallback: trimming happens before expansion to RGBA.
    gli::texture large(gli::TARGET_2D,gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16,{1024,1024,1},1,1,11);
    std::memset(large.data(),0,large.size());
    std::vector<char> large_bytes;assert(gli::save_dds(large,large_bytes));
    DdsTexture large_full,large_small;std::string large_error;
    assert(decode_dds(large_bytes.data(),large_bytes.size(),false,large_full,large_error));
    assert(decode_dds(large_bytes.data(),large_bytes.size(),false,large_small,large_error,256));
    assert(large_small.extent.width==256 && large_small.mip_levels==9);
    assert(large_small.pixels.size()*15<large_full.pixels.size());
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
